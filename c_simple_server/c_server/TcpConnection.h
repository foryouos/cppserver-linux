#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

// �������߲�ͬ���ͷ�ʽ��
// 1,ȫ���ŵ��������ٷ��ͣ�2����ű߷���
// ������ǵ�һ��
// û���弴Ĭ�ϵڶ���
// #define MSG_SEND_AUTO

struct TcpConnection
{
	struct EventLoop* evLoop;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	char name[32];
	//httpЭ��
	struct HttpRequest* request;
	struct HttpResponse* response;
};
//��ʼ��
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evloop);
// ��Դ����
int tcpConnectionDestory(void* arg);