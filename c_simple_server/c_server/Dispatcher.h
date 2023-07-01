#pragma once
#include "Channel.h"

//Dispatcher �ṹ��
struct EventLoop;  //�ȶ������������������໥��������³��ֵ���������������
struct Dispatcher
{
	//init ��ʼ��epoll��poll��select����Ҫ�����ݿ飬���ز�ͬ�����ݿ����ͣ�
	// ʹ�÷��� ת���ɶ�Ӧ������
	void* (*init)();  //poll poll_fd���� /epoll epoll_event //select fd_set ����ʼ��������
	//��� �Ѵ������ļ���������ӵ���Ӧ������epoll/poll/select����
	// ͨ��EventLoop�Ϳ���ȡ��Dispatcher
	 //ChannelIO��·ģ��
	int (*add)(struct Channel* channel,struct EventLoop* evLoop); 
	//ɾ�� ��ĳһ���ڵ��epoll����ɾ��
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	//�޸�
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	//�¼���⣬ ���ڼ����������֮һģ��epoll_wait�ȵ�һϵ���¼����Ƿ����¼��������/д�¼�
	int (*dispatch)(struct EventLoop* evLoop,int timeout);//��λ S ��ʱʱ��
	//�������(�ر�fd�����ͷ��ڴ�)
	int (*clear)(struct EventLoop* evLoop);
};