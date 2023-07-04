#pragma once
#include "Channel.h"
#include <string>
using namespace std;
//Dispatcher �ṹ��
class EventLoop;  //�ȶ������������������໥��������³��ֵ���������������
//Epoll,Poll,Selectģ��
class Dispatcher
{

public:
	Dispatcher(struct EventLoop* evLoop);
	virtual ~Dispatcher();  //Ҳ�麯�����ڶ�̬ʱ
	virtual int add();   //���� =���麯�����Ͳ��ö���
	//ɾ�� ��ĳһ���ڵ��epoll����ɾ��
	virtual int remove();
	//�޸�
	virtual int modify();
	//�¼���⣬ ���ڼ����������֮һģ��epoll_wait�ȵ�һϵ���¼����Ƿ����¼��������/д�¼�
	virtual int dispatch(int timeout = 2);//��λ S ��ʱʱ��
	inline void setChannel(Channel* channel)
	{
		m_channel = channel;
	}
protected: // ����Ȩ���ܱ����ģ����ܱ��ⲿ���� �������Ա�����̳У�
	string m_name = string(); //Ϊ��ʵ����������
	Channel* m_channel;
	EventLoop* m_evLoop;
};