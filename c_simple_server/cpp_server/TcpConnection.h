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

// �������߳���ͻ��˽���ͨ�ţ��ֱ�洢���д���ٻص�����->�������buffer���������ص�ͨ�Ź���
class TcpConnection
{
public:
	TcpConnection(int fd, EventLoop* evloop);
	~TcpConnection();
	static int processRead(void* arg);  //���ص�
	static int processWrite(void* arg);  //д�ص�
	static int destory(void* arg);       //���ٻص�

private:
	string m_name;
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;  //��������
	Buffer* m_writeBuf; //д������
	//httpЭ��
	HttpRequest* m_request;  //http����
	HttpResponse* m_response;  //http��Ӧ
};
