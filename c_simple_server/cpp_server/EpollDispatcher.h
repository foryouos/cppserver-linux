#pragma once
#include "Channel.h"
#include "Dispatcher.h"
#include <string>
#include <sys/epoll.h>

using namespace std;
//Dispatcher �ṹ��
class EpollDispatcher : public Dispatcher  //�̳и���Dispatcher
{

public:
	EpollDispatcher(struct EventLoop* evLoop);
	~EpollDispatcher();  //Ҳ�麯�����ڶ�̬ʱ
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
	int epollCtl(int op);

private:
	int m_epfd; //epoll�ĸ��ڵ�
	//��������
	struct epoll_event* m_events;
	const int m_maxNode = 520;
};