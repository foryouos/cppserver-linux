#define _GNU_SOURCE 
#include "HttpRequest.h"
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "log.h"
#define HeaderSize 12
struct HttpRequest* httpRequestInit()
{
	struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	httpRequetReset(request);
	// 结构体数据，分配若干个元素
	request->reqheaders = (struct RequestHeader*)malloc(sizeof(struct RequestHeader)* HeaderSize);
	
	return request;
}

void httpRequetReset(struct HttpRequest* req)
{
	req->curState = ParseReqLine;
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	// 结构体体数据，分配若干个元素
	req->reqHeadersNum = 0;
}

void httpRequetResetEx(struct HttpRequest* req)
{
	free(req->url);
	free(req->method);
	free(req->version);
	// 释放调reqHeader数据
	if (req->reqheaders != NULL)
	{
		for (int i = 0; i < req->reqHeadersNum; ++i)
		{
			free(req->reqheaders[i].key);
			free(req->reqheaders[i].value);
		}
		free(req->reqheaders);
	}
	httpRequetReset(req);

}

void httpRequetDestory(struct HttpRequest* req)
{
	if (req != NULL)
	{
		httpRequetResetEx(req);
		
		free(req);
	}
	//若等于空函数结束
}

enum HttpRequestState httpRequestState(struct HttpRequest* request)
{
	return request->curState;
}

void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value)
{
	request->reqheaders[request->reqHeadersNum].key = (char*)key;
	request->reqheaders[request->reqHeadersNum].value = (char*)value;
	request->reqHeadersNum++;
}

char* httpRequestGetHeader(struct HttpRequest* request, const char* key)
{

	if (request != NULL)
	{
		for (int i = 0; i < request->reqHeadersNum; ++i)
		{
			//strncasecmp 比较n个字节的字符串是否相等不区分大小写
			if (strncasecmp(request->reqheaders[i].key, key, strlen(key)) == 0)
			{
				return request->reqheaders[i].value; 
			}
		}
	}

	return NULL;
}

//拆分请求行
// 传入一级指针，指针(4个字节)在传递过程中会产生一个副本，是指针产生一个副本，而不是指针的地址
// 在函数内部给外部的一级指针分配内存，把外部一级指针的地址传给参数即二级指针，
// 返回 结束的指针
char* splitRequestLine(const char* start,const char* end,const char*sub,char**ptr)
{
	char* space = end; //求长度，尾指针 - 开始指针
	if (sub!=NULL)
	{
		space = memmem(start, end-start, sub, strlen(sub));
		assert(space != NULL); //取出的内存块不为空
	}
	// 请求方式的长度
	int length = space - start;
	// 传指针，传指针的地址(二级指针)，而不是指针
	// 给某个指针赋值
	char* tmp = (char*)malloc(length + 1); 
	strncpy(tmp,start,length);
	tmp[length] = '\0';
	//*ptr 二级指针解引用，变为一级指针
	*ptr = tmp;  //*ptr之后变成一级指针
	return space + 1;
}

bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer* readBuf)
{
	//读出请求行 保存字符串结束地址
	char* end = bufferFindCRLF(readBuf);
	// 保存字符串的起始地址
	char* start = readBuf->data + readBuf->readPos;
	// 得出请求行的总长度
	int lineSize = end - start;
	// 
	if (lineSize)
	{
		start = splitRequestLine(start, end, " ", &request->method);
		start = splitRequestLine(start, end, " ", &request->url);
		splitRequestLine(start, end, NULL, &request->version);
#if 0
		// get / xxx/xx.txt http/1.1
		// 请求方式
		char* space = memmem(start, lineSize, " ", 1);
		assert(space != NULL); //取出的内存块不为空
		// 请求方式的长度
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';   

		// 请求的静态资源
		start = space + 1;
		char* space = memmem(start, end-start, " ", 1);
		assert(space != NULL); //取出的内存块不为空
		// 请求方式的长度
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->method[urlSize] = '\0';     

		// 请求行里请求协议版本
		start = space + 1;
		// char* space = memmem(start, end - start, " ", 1);
		// assert(space != NULL); //取出的内存块不为空
		// 请求方式的长度
		// int urlSize = space - start;
		request->version = (char*)malloc(end-start + 1);
		strncpy(request->version, start, end - start);
		request->method[end - start] = '\0'; 
#endif

		// 为解析请求头做准备 
		readBuf->readPos += lineSize; //移动读指针
		readBuf->readPos += 2;        // 读指针
		// 修改状态
		request->curState = ParseReqHeaders; //请求头解析成功
		return true;
	}
	return false;
}
// readBuf请求行已经更新到起始位置
bool parseHttpRequestHeader(struct HttpRequest* request,struct Buffer* readBuf)
{
	char* end = bufferFindCRLF(readBuf);
	if (end != NULL)
	{
		//通过 : 来读取前后的数据
		// 开始位置
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		//基于:搜索字符串
		char* middle = memmem(start, lineSize, ": ",2); //标准的是: 加空格的，浏览器都是标准的
		if (middle != NULL)
		{
			char* key = malloc(middle - start + 1);  // 1为\0完整的字符串
			strncpy(key, start, middle - start);
			key[middle - start] = '\0';

			char* value = malloc(end-middle-2 + 1);  // 2为：
			strncpy(value, middle+2, end - middle - 2);
			value[end - middle - 2] = '\0';

			httpRequestAddHeader(request, key, value);
			// 指针移动到下一行
			readBuf->readPos += lineSize;
			readBuf->readPos += 2;
		}
		else
		{
			//如果middle为空，请求头被解析完毕,跳过空行
			readBuf->readPos += 2;
			// 修改解析状态
			// TODO忽略post请求，按照get请求处理
			request->curState = ParseReqDone;

		}

		return true;
	}
	return false;
}

bool parseHttpRequest(struct HttpRequest* request,struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	bool flag = true;
	// 先解析请求行
	while (request->curState != ParseReqDone)
	{
		// 根据请求头目前的请求状态进行选择
		switch (request->curState)
		{
		case ParseReqLine:
			flag = parseHttpRequestLine(request, readBuf);
			break;
		case ParseReqHeaders:
			flag = parseHttpRequestHeader(request, readBuf);
			break;
		case ParseReqBody: //post的请求，咱不做处理
			// 读取post数据
			break;
		default:
			break;
		}
		if (!flag)
		{
			return flag;
		}
		//判断是否解析完毕，如果完毕，需要准备回复的数据
		if (request->curState == ParseReqDone)
		{
			// 1，根据解析出的原始数据，对客户端请求做出处理
			processHttpRequest(request,response);
			// 2,组织响应数据并发送
			httpResponsePrepareMsg(response, sendBuf, socket);
		}
	}
	// 状态还原，保证还能继续处理第二条及以后的请求
	request->curState = ParseReqLine;
	//再解析请求头
	return flag;
}
// 将字符转换为整形数
int hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;  //+10进位
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}
// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from) //
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			*to = (char)(hexToDec(from[1]) * 16 + hexToDec(from[2]));  //将两个数转为十进制

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2; //调到下一个%
		}
		else
		{
			// 字符拷贝, 赋值
			*to = *from;
		}

	}
	*to = '\0';
}
const char* getFileType(const char* name)
{
	//a.jpg,a.mp4,a.html
	//从右向左查找 "."字符，如不存在返回NULL
	const char* dot = strrchr(name, '.'); //strrchr 从右往左找.,把.到右边的后缀放入name
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}
// 处理基于get的http请求
bool processHttpRequest(struct HttpRequest* request,struct HttpResponse* response)
{
	
	if (strcasecmp(request->method, "get") != 0)   //strcasecmp比较时不区分大小写
	{
		//非get请求不处理
		return -1;
	}
	decodeMsg(request->url, request->url); // 避免中文的编码问题 将请求的路径转码 linux会转成utf8
	//处理客户端请求的静态资源(目录或文件)
	char* file = NULL;
	if (strcmp(request->url, "/") == 0) //判断是不是根目录
	{
		file = "./";
	}
	else
	{
		file = request->url + 1;  // 指针+1 把开始的 /去掉吧

	}
	//判断file属性，是文件还是目录
	struct stat st;
	int ret = stat(file, &st); // file文件属性，同时将信息传入st保存了文件的大小
	if (ret == -1)
	{
		//文件不存在  -- 回复404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd); //发送404对应的html文件
		strcpy(response->fineName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->statusMsg, "Not Found");
		// 响应头
		httpResponseAddHeader(response, "content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;

	}
	strcpy(response->fineName, file);
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");

	//判断文件类型
	if (S_ISDIR(st.st_mode)) //如果时目录返回1，不是返回0
	{
		//把这个目录中的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), (int)st.st_size);
		//sendDir(file, cfd);
		// 响应头
		httpResponseAddHeader(response, "content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else
	{
		//把这个文件的内容发给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		//sendFile(file, cfd);
		// 响应头
		char tmp[12] = { 0 };
		// TODo修改文件内存 格式
		sprintf(tmp,"%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));
		httpResponseAddHeader(response, "Content-length", tmp);
		response->sendDataFunc = sendFile;
	}
	return false;
}


//要回复http响应的数据块
void sendFile(const char* filename, struct Buffer* sendBuf, int cfd)
{
	//读一部分发一部分，发送数据底层使用TCP协议
	//1，打开文件
	int fd = open(filename, O_RDONLY);
	//判断文件是否打开成功
	assert(fd > 0); //如果大于0没有问题，若不大于0，程序异常失败

#if 0
	//读数据
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof(buf));
		if (len > 0)
		{
			//send(cfd, buf, len, 0); //发送数据给浏览器
			// 将数据添加到发送缓存中区
			bufferAppendData(sendBuf, buf, len);
			// 即存即发
			// #ifndef是"if not defined"
			// #ifndef如果没有定义这个宏，下面就是有效的，如果定义了，就是无效的
#ifndef MSG_SEND_AUTO
			bufferSendData(sendBuf, cfd);

			
#endif // !MSG_SEND_AUTO
			//减慢发送节奏
			//usleep(10); // 服务器休眠微妙
		}
		else if (len == 0)  //文件已经读完了
		{
			break;
		}
		else
		{
			close(fd);
			perror("read");
		}
	}
#else
	// TODO
	off_t offset = 0; //sendfile的偏移量，判断是否将数据发送完毕
	int size = (int)lseek(fd, 0, SEEK_END); //从0到某位的文件偏移量，即文件有多少个字节
	lseek(fd, 0, SEEK_SET); //再把指针移动到首部
	//系统函数，发送文件，linux内核提供的sendfile 也能减少拷贝次数
	// sendfile发送文件效率高，而文件目录使用send
	//通信文件描述符，打开文件描述符，fd对应的文件偏移量一般为空，
	//循环发送
	int count = 0;
	while (offset < size)
	{
		//int ret = bufferSendFileData(cfd, fd,offset, size);
		int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));
		if (ret == -1 && errno == EAGAIN)
		{
			printf("not data ....");
			perror("sendfile");
		}
		count += ret;
		//Debug("cout: %d,offset:%d",count, offset);
		
	}
	
		
	//while (offset < size)
	//{
	//	// int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));  //单独单文件出现发送不全，offset会自动修改当前读取位置
	//	bufferSendFileData(sendBuf, cfd, fd);
	//	printf("ret value: %d\n", ret);
	//	if (ret == -1 && errno == EAGAIN)
	//	{
	//		printf("not data ....");
	//		perror("sendfile");
	//	}
	//}

#endif
	close(fd); //关闭打开的文件
}
/*
<html>
	<head>
		<title>test</title>
	</head>
<body>
	<table>
		<tr>
			<td></td>
			<td></td>
		</tr>
		<tr>
			<td></td>
			<td></td>
		</tr>
	</table>
</body>
</html>
*/
// 把添加好的数据块放到发送缓存区中
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><style type=\"text/css\">a{text-decoration:none};</style>\
		<body style=\"color: #4f6b72;background: #E6EAE9;\"><table style=\"margin: 10px;\">", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort); //返回目录下有多少个文件
	for (int i = 0; i < num; ++i)
	{
		//取出文件名 namelist指向一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		//判断文件是不是目录
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		//得到文件属性
		stat(subPath, &st); //name只是文件名称，需要拼接相对路径 
		if (S_ISDIR(st.st_mode))  //是不是目录
		{

			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			//文件a标签不需要加斜杠
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}

		//send(cfd, buf, strlen(buf), 0);
		bufferAppendString(sendBuf, buf); //将拼接好的数据发送到缓冲区中
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd);
#endif // !MSG_SEND_AUTO
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	//把html结束的标签发送给字符串
	sprintf(buf, "</table></body></html>");
	// send(cfd, buf, strlen(buf), 0);
	bufferAppendString(sendBuf, buf); //将拼接好的数据发送到缓冲区中
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd);
#endif // !MSG_SEND_AUTO
	free(namelist);
}

