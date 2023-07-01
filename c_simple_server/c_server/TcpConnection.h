#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

// 区分两者不同发送方式，
// 1,全部放到缓存区再发送，2，变放边发送
// 定义就是第一种
// 没定义即默认第二种
// #define MSG_SEND_AUTO

struct TcpConnection
{
	struct EventLoop* evLoop;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	char name[32];
	//http协议
	struct HttpRequest* request;
	struct HttpResponse* response;
};
//初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evloop);
// 资源销毁
int tcpConnectionDestory(void* arg);