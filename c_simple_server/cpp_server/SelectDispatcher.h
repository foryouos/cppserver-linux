#pragma once
#include "Channel.h"
#include "Dispatcher.h"
#include <string>
#include <sys/select.h>

using namespace std;
//Dispatcher �ṹ��
class SelectDispatcher : public Dispatcher  //�̳и���Dispatcher
{
public:
	SelectDispatcher(struct EventLoop* evLoop);
	~SelectDispatcher();  //Ҳ�麯�����ڶ�̬ʱ
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
	void setFdSet();
	void clearFdset();
private:
	fd_set m_readSet;
	fd_set m_writeSet;
	const int m_maxSize = 1024;
};