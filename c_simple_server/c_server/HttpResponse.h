#pragma once
#include "Buffer.h"
// 定义状态码枚举
enum HttpStatusCode
{
	Unknown,
	OK = 200,
	MovedPermannently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};
// 定义响应的结构体 键值对
struct ResponseHeader
{
	char key[32];
	char value[128];
};
// 定义一个函数指针，用来组织要回复给客户端的数据块
// fileName要给客户端回复的静态资源内容
// sendBuf 内存，存储发送数据的Buffer缓冲
// socket 文件描述符，用于通信，将sendBuf数据发送给客户端
typedef void(*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);


//定义结构体
struct HttpResponse
{
	// 状态行：状态码，状态描述，http协议
	enum HttpStatusCode statusCode;
	char statusMsg[128];  //状态描述数组
	char fineName[128];  // 文件名
	// 响应头 - 键值对(存储响应头内部所有数据

	struct ResponseHeader* headers;
	int headerNum; //响应头的个数

	responseBody sendDataFunc;
};

//初始化HttpResponse结构体的初始化函数
struct HttpResponse* httpResponseInit();
// 销毁函数
void httpResponseDestory(struct HttpResponse* response);

// 添加响应头
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value);
// 组织http响应数据
// HttpResponse  组织数据块，写到内存中，写到sendBuf
// sendBuf 发送数据的 socket用于通信
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);