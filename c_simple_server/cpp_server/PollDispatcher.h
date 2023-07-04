#pragma once
#include "Channel.h"
#include "Dispatcher.h"
#include <string>
#include <poll.h>

using namespace std;
//Dispatcher �ṹ��
class PollDispatcher : public Dispatcher  //�̳и���Dispatcher
{

public:
	PollDispatcher(struct EventLoop* evLoop);
	~PollDispatcher();  //Ҳ�麯�����ڶ�̬ʱ
	// override����ǰ��ĺ�������ʾ�˺����ǴӸ���̳й����ĺ��������ཫ��д�����麯��
	// override���Զ���ǰ������ֽ��м��,
	int add() override;   //���� =���麯�����Ͳ��ö��� 
	//ɾ�� ��ĳһ���ڵ��epoll����ɾ��
	int remove() override;
	//�޸�
	int modify() override;
	//�¼���⣬ ���ڼ����������֮һģ��epoll_wait�ȵ�һϵ���¼����Ƿ����¼��������/д�¼�
	int dispatch(int timeout = 2) override;//��λ S ��ʱʱ��
	// ���ı�Ĳ�д��ֱ�Ӽ̳и���
private:
	int m_maxfd;  //�ļ����������ֵ//maxfd��Ҫ�����ұߵı߽�
	//��������
	struct pollfd *m_fds;
	const int m_maxNode = 1024;

};