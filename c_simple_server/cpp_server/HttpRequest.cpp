//#define _GNU_SOURCE 
#include "HttpRequest.h"
#include "TcpConnection.h"
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
#include <sys/sendfile.h>
HttpRequest::HttpRequest()
{
	Reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::Reset()
{
	m_curState = PressState::ParseReqLine;
	m_method = m_url = m_version = string();
	m_reqHeaders.clear();

}


void HttpRequest::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(const string key)
{
	auto item = m_reqHeaders.find(key);
	if (item == m_reqHeaders.end())  //判断是否搜索到，
	{
		//没有搜索到
		return string();
	}
	// 搜索到了，first是key值second value值
	return item->second; //取value值
}
bool HttpRequest::parseRequestLine(Buffer* readBuf)
{
	//读出请求行 保存字符串结束地址
	char* end = readBuf->findCRLF();
	// 保存字符串的起始地址
	char* start = readBuf->data();
	// 得出请求行的总长度
	int lineSize = end - start;
	// 
	if (lineSize > 0)
	{
		auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", methodFunc);
		auto urlFunc = bind(&HttpRequest::seturl, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", urlFunc);
		auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
		splitRequestLine(start, end, NULL, versionFunc);
		// 为解析请求头做准备 
		readBuf->readPosIncrease(lineSize+2); //移动读指针
		// 修改状态
		setState(PressState::ParseReqHeaders);
		 //请求头解析成功
		return true;
	}
	return false;
}

bool HttpRequest::parseRequestHeader(Buffer* readBuf)
{
	char* end = readBuf->findCRLF();
	if (end != nullptr)
	{
		//通过 : 来读取前后的数据
		// 开始位置
		char* start = readBuf->data();
		int lineSize = end - start;
		//基于:搜索字符串
		char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2)); //标准的是: 加空格的，浏览器都是标准的
		if (middle != nullptr)
		{
			int keyLen = middle - start;
			int valueLen = end - middle - 2;
			if (keyLen > 0 && valueLen > 0)
			{
				string key(start,keyLen);
				string value(middle+2,valueLen);
				addHeader(key, value);
			}
			// 指针移动到下一行
			readBuf->readPosIncrease(lineSize + 2);
		}
		else
		{
			//如果middle为空，请求头被解析完毕,跳过空行
			readBuf->readPosIncrease(2);
			// 修改解析状态
			// TODO忽略post请求，按照get请求处理
			setState(PressState::ParseReqDone);
		}

		return true;
	}
	return false;
}

bool HttpRequest::parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket)
{
	bool flag = true;
	// 先解析请求行
	while (m_curState !=PressState::ParseReqDone)
	{
		// 根据请求头目前的请求状态进行选择
		switch (m_curState)
		{
		case PressState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case PressState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case PressState::ParseReqBody: //post的请求，咱不做处理
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
		if (m_curState == PressState::ParseReqDone)
		{
			// 1，根据解析出的原始数据，对客户端请求做出处理
			processHttpRequest(response);
			// 2,组织响应数据并发送
			response->prepareMsg(sendBuf, socket);
		}
	}
	// 状态还原，保证还能继续处理第二条及以后的请求
	m_curState = PressState::ParseReqLine;
	//再解析请求头
	return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse* response)
{
	if (strcasecmp(m_method.data(), "get") != 0)   //strcasecmp比较时不区分大小写
	{
		//非get请求不处理
		return -1;
	}
	m_url = decodeMsg(m_url); // 避免中文的编码问题 将请求的路径转码 linux会转成utf8
	//处理客户端请求的静态资源(目录或文件)
	const char* file = NULL;
	if (strcmp(m_url.data(), "/") == 0) //判断是不是根目录
	{
		file = "./";
	}
	else
	{
		file = m_url.data() + 1;  // 指针+1 把开始的 /去掉吧

	}
	//判断file属性，是文件还是目录
	struct stat st;
	int ret = stat(file, &st); // file文件属性，同时将信息传入st保存了文件的大小
	if (ret == -1)
	{
		//文件不存在  -- 回复404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd); //发送404对应的html文件
		response->setFileName("404.html");
		response->setStatusCode(StatusCode::NotFound);
		// 响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;

	}
	response->setFileName(file);
	response->setStatusCode(StatusCode::OK);
	//判断文件类型
	if (S_ISDIR(st.st_mode)) //如果时目录返回1，不是返回0
	{
		//把这个目录中的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), (int)st.st_size);
		//sendDir(file, cfd);
		// 响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else
	{
		//把这个文件的内容发给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		//sendFile(file, cfd);
		// 响应头
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", to_string(st.st_size));
		response->sendDataFunc = sendFile;
	}
	return false;
}

string HttpRequest::decodeMsg(string msg)
{
	string str = string();
	const char* from = msg.data();
	for (; *from != '\0';++from) //
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			//将两个数转为十进制
			str.append(1, (char)(hexToDec(from[1]) * 16 + hexToDec(from[2])));

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2; //调到下一个%
		}
		else
		{
			// 字符拷贝, 赋值
			str.append(1, *from);
		}

	}
	str.append(1, '\0'); //想要的数据存放到str当中
	return str;
}

const string HttpRequest::getFileType(const string name)
{
	//string(const char*) //编译器会调用这个类的构造函数进行隐式类型转换
	//a.jpg,a.mp4,a.html
	//从右向左查找 "."字符，如不存在返回NULL
	const char* dot = strrchr(name.data(), '.'); //strrchr 从右往左找.,把.到右边的后缀放入name
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

int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;  //+10进位
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

void HttpRequest::sendFile(string filename, Buffer* sendBuf, int cfd)
{
	//读一部分发一部分，发送数据底层使用TCP协议
//1，打开文件
	int fd = open(filename.data(), O_RDONLY);
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
			sendBuf->appendString(buf);
			// 即存即发
			// #ifndef是"if not defined"
			// #ifndef如果没有定义这个宏，下面就是有效的，如果定义了，就是无效的
#ifndef MSG_SEND_AUTO
			sendBuf->sendData(cfd);


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
	sendBuf->sendData(cfd, fd, offset, size);
	//while (offset < size)
	//{
	//	//int ret = bufferSendFileData(cfd, fd,offset, size);
	//	int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));
	//	if (ret == -1 && errno == EAGAIN)
	//	{
	//		printf("not data ....");
	//		perror("sendfile");
	//	}
	//}

#endif
	close(fd); //关闭打开的文件
}

void HttpRequest::sendDir(string dirName, Buffer* sendBuf, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><style type=\"text/css\">a{text-decoration:none};</style>\
		<body style=\"color: #4f6b72;background: #E6EAE9;\"><table style=\"margin: 10px;\">", dirName.data());
	struct dirent** namelist;
	int num = scandir(dirName.data(), &namelist, NULL, alphasort); //返回目录下有多少个文件
	for (int i = 0; i < num; ++i)
	{
		//取出文件名 namelist指向一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		//判断文件是不是目录
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName.data(), name);
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
		sendBuf->appendString(buf); //将拼接好的数据发送到缓冲区中
#ifndef MSG_SEND_AUTO
		sendBuf->sendData(cfd);
#endif // !MSG_SEND_AUTO
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	//把html结束的标签发送给字符串
	sprintf(buf, "</table></body></html>");
	// send(cfd, buf, strlen(buf), 0);
	sendBuf->appendString(buf); //将拼接好的数据发送到缓冲区中
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(cfd);
#endif // !MSG_SEND_AUTO
	free(namelist);
}

char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback)
{
	char* space = const_cast<char*>(end); //求长度，尾指针 - 开始指针
	if (sub != nullptr)
	{
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
		assert(space != nullptr); //取出的内存块不为空
	}
	// 请求方式的长度
	int length = space - start;
	callback(string(start, length));
	return space + 1;
}
