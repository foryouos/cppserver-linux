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

// 负责子线程与客户端进行通信，分别存储这读写销毁回调函数->调用相关buffer函数完成相关的通信功能
class TcpConnection
{
public:
	TcpConnection(int fd, EventLoop* evloop);
	~TcpConnection();
	static int processRead(void* arg);  //读回调
	static int processWrite(void* arg);  //写回调
	static int destory(void* arg);       //销毁回调

private:
	string m_name;
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;  //读缓存区
	Buffer* m_writeBuf; //写缓存区
	//http协议
	HttpRequest* m_request;  //http请求
	HttpResponse* m_response;  //http响应
};
