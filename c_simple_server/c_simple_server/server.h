#pragma once
//初始化监听的套接字
int initListenFd(unsigned short port);
//启动epoll
int epollRun(int lfd);
//和客户端建立连接
void* acceptClient(void* arg);
// 接受http请求的消息
void* recvHttpRequest(void* arg);
//解析请求行
int parseRequestLine(const char* line, int cfd); //解析的数据，以及用于通信的文件描述符
//发送文件
int sendFile(const char* filename, int cfd);
// 发送响应头(状态行+响应头) 
// 通信文件描述符，状态码，状态描述，响应头，类型，长度
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);
//根据文件后缀，得到文件类型
const char* getFileType(const char* name);
//发送目录
int sendDir(const char* dirName, int cfd);
// 将十六进制转为10进制
int hexToDec(char c);
//中文字符解码
void decodeMsg(char* to, char* from);
