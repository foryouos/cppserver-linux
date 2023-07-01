#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
//请求键值对
struct RequestHeader
{
	char* key;
	char* value;
};
// 当前的解析状态
enum HttpRequestState
{
	ParseReqLine,     //当前解析的是请求行
	ParseReqHeaders,  // 当前解析的是请求头
	ParseReqBody,     // 当前解析的是请求的数据块
	ParseReqDone,     // 当前Http请求已经解析完了

};

// 定义http 请求结构体
struct HttpRequest
{
	char* method; //请求方法
	char* url;    //请求连接
	char* version;//请求版本
	struct RequestHeader* reqheaders; //多个
	int reqHeadersNum;    //存储多少个有效的请求键值对
	enum HttpRequestState curState;
};

// 初始化函数
struct HttpRequest* httpRequestInit();
// 重置http请求结构体
void httpRequetReset(struct HttpRequest* req);
// 释放内存重置
void httpRequetResetEx(struct HttpRequest* req);
// 内存释放
void httpRequetDestory(struct HttpRequest* req);
// 获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest* request);
//添加请求头
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
// 根据key得到请求头的value
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);
// 解析请求行,bool表示失败与成果，从readBud读缓存中读取数据，三部分，请求方式，url，和http协议
bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer* readBuf);
// 解析请求头，请求一行（多行，多次调用即可)
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
// 解析http请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket);
// 处理Http请求协议 Get的http请求
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from);
//根据文件后缀，得到文件类型
const char* getFileType(const char* name);
// 将字符转换为整形数
int hexToDec(char c);
//要回复http响应的数据块
void sendFile(const char* filename, struct Buffer* sendBuf, int cfd);
// 把添加好的数据块放到发送缓存区中
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);