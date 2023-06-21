#include "server.h"
#include <arpa/inet.h>  //socket需要
#include <sys/epoll.h>  //epoll需要
#include <stdio.h>      //NULL头文件
#include <unistd.h>
#include <fcntl.h>  //fcntl
#include <string.h>  //memcpy
#include <errno.h>  //errno
#include <strings.h>  //strcasecmp
#include <sys/stat.h>
#include <assert.h>  //断言
#include <unistd.h>  //usleep
#include <sys/sendfile.h>  //sendfile
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
//传参结构体
struct FDInfo
{
	int fd;
	int epfd;
	pthread_t tid;
}FDInfo;

//传入端口
int initListenFd(unsigned short port)
{
	//1，创建监听的fd
	// AF_INET 基于IPV4，0 流式协议中的TCP协议
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket"); //创建失败
		return -1;
	}
	//2，设置端口复用
	int opt = 1; //1端口复用
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt"); //创建失败
		return -1;
	}
	//3，绑定端口
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //IPV4协议
	addr.sin_port = htons(port); //将端口转为网络字节序
	addr.sin_addr.s_addr = INADDR_ANY; //本地所有IP地址
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind"); //失败
		return -1;
	}
	//4，设置监听  一次性可以和多少客户端连接,内核最大128，若给很大，内核会改为128
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	printf("Init successful....\n");
	//返回fd
	return lfd;
}

int epollRun(int lfd)
{
	//1，创建epoll实例
	int epfd = epoll_create(1); //参数大于0即可，没有实际意义
	if (epfd == -1)
	{
		perror("epoll_Create");
		return -1;
	}
	// 2,lfd 上树
	struct epoll_event ev;
	ev.data.fd = lfd; //指定监测的文件描述符
	ev.events = EPOLLIN; //检测是否有新连接
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	//printf("epoll_stl up open...!\n");
	// 3,检测，是一个持续的过程
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event); //总长度/除以单个大小，得出个数
	while (1)
	{

		int num = epoll_wait(epfd, evs, size, -1); //阻塞时间-1没有事件触发，就一直阻塞
		//printf("epoll_wait open...!\n");
		//取出
		for (int i = 0; i < num; ++i)
		{
			int fd = evs[i].data.fd;
			struct FDInfo* info = (struct FDInfo*)malloc(sizeof(struct FDInfo));
			info->fd = fd;
			info->epfd = epfd;
			//判断是不是用于监听的
			if (fd == lfd)
			{
				//建立新连接 accept 有连接就不会阻塞，建立连接，并将新的文件描述符添加到epoll当中
				//acceptClient(fd, epfd);

				pthread_create(&info->tid, NULL, acceptClient, info);
			}
			else
			{
				//判断是接受接收端数据,数据为http请求的格式，
				//recvHttpRequest(fd, epfd);

				pthread_create(&info->tid, NULL, recvHttpRequest, info);
			}

		}

	}
	return 0;
}

void* acceptClient(void* arg)
{
	struct FDInfo* info = (struct FDInfo*)arg;
	//printf("start connection ....\n");
	//1，建立连接
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return NULL;
	}
	//使用epoll的边沿模式
	//2,设置非阻塞
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;  //按位或让其属性存在
	fcntl(cfd, F_SETFL, flag); //添加到cfd当中，F_SETFL设置文件状态
	//3,cfd添加到epoll当中
	struct epoll_event ev;
	ev.data.fd = cfd; //指定监测的文件描述符
	ev.events = EPOLLIN | EPOLLET; //检测是否有读事件,边沿模式
	//添加到epoll当中
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return NULL;
	}
	printf("recvMsg threadId : %ld\n", info->tid);
	free(info);

	return NULL;
}
void* recvHttpRequest(void* arg)
{
	struct FDInfo* info = (struct FDInfo*)arg;
	printf("recv message....\n");
	// 1，把用户所有数据先存到本地
	char buf[4096] = { 0 }; //总
	char tmp[1024] = { 0 }; //临时
	//边缘模式，需要一次把所有数据全部读完
	int len = 0, total = 0; //len每次接受的数据长度，total为总长度
	while ((len = (int)recv(info->fd, tmp, sizeof tmp, 0)) > 0)
	{
		//printf("recv while total\n");
		if (total + len < sizeof(buf))
		{
			memcpy(buf + total, tmp, (size_t)len);  //修改复制地址
		}
		total += len;

	}
	printf("total = %d\n", total);
	//判断数据是否被接受完毕
	if (len == -1 && errno == EAGAIN) //len 等于-1 并且错误为EAGAIN
	{
		printf("analyse get request!\n");
		//解析请求行,对，先处理get请求
		char* pt = strstr(buf, "\r\n"); // 遇到这两个字符\r\n 从左往右搜到之后，就结束了
		int reqLen = (int)(pt - buf); //得到请求行的长度，结束的地址，减去起始的地址
		buf[reqLen] = '\0'; // buf[reqLen-1]是上面的最后一个字符
		parseRequestLine(buf, info->fd);
	}
	else if (len == 0)
	{
		printf("client closed!\n");
		//客户端断开了连接，删除对应的文件描述符
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else
	{
		//printf("recvHttpRequest error len = %d\n", len);
		perror("recv");
	}
	printf("recvMsg threadId : %ld\n", info->tid);
	free(info);
	return NULL;
}

int parseRequestLine(const char* line, int cfd)
{
	//解析请求行， get / xxx/1.jpg http1.1
	// 三部分:请求方式，请求资源，请求http协议的版本，数据之间有空格，
	// sscanf 对格式化的字符串进行拆分
	char method[12];  //get 或post
	char path[1024];  //存储请求的静态资源，文件路径
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("method: %s,path: %s\n", method, path);
	if (strcasecmp(method, "get") != 0)   //strcasecmp比较时不区分大小写
	{
		//非get请求不处理
		return -1;
	}
	decodeMsg(path, path); //将请求的路径转码 linux会转成utf8
	//处理客户端请求的静态资源(目录或文件)
	char* file = NULL;
	if (strcmp(path, "/") == 0) //判断是不是根目录
	{
		file = "./";
	}
	else
	{
		file = path + 1;  //把开始的 /去掉吧

	}
	//判断file属性，是文件还是目录
	struct stat st;
	int ret = stat(file, &st); //file文件属性，同时st保存了文件的大小
	if (ret == -1)
	{
		//文件不存在  -- 回复404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		sendFile("404.html", cfd);
		return 0;

	}
	//判断文件类型
	if (S_ISDIR(st.st_mode)) //如果时目录返回1，不是返回0
	{
		//把这个目录中的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else
	{
		//把这个文件的内容发给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		sendFile(file, cfd);
	}

	return 0;
}

//要回复http响应的数据块
int sendFile(const char* filename, int cfd)
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
			send(cfd, buf, len, 0); //发送数据给浏览器
			//减慢发送节奏
			usleep(10); // 服务器休眠微妙
		}
		else if (len == 0)  //文件已经读完了
		{
			break;
		}
		else
		{
			perror("read");
		}
	}
#else
	off_t offset = 0; //sendfile的偏移量，判断是否将数据发送完毕
	int size = (int)lseek(fd, 0, SEEK_END); //从0到某位的文件偏移量，即文件有多少个字节
	lseek(fd, 0, SEEK_SET); //再把指针移动到首部
	//系统函数，发送文件，linux内核提供的sendfile 也能减少拷贝次数
	//通信文件描述符，打开文件描述符，fd对应的文件偏移量一般为空，
	while (offset < size)
	{
		int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));  //单独单文件出现发送不全，offset会自动修改当前读取位置
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("not data ....");
			perror("sendfile");
		}
	}

#endif
	close(fd); //关闭打开的文件
	return 0;
}

//发送文件之前先调用
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	// 状态行
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr); //换行对应的\r\n
	//响应头
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type); //buf + strlen(buf) 指针后移
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length); //最后一行是空行

	send(cfd, buf, strlen(buf), 0);
	return 0;
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
int sendDir(const char* dirName, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int name = scandir(dirName, &namelist, NULL, alphasort); //返回目录下有多少个文件
	for (int i = 0; i < name; ++i)
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

		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	//把html结束的标签发送给字符串
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
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
