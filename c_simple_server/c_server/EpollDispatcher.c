#include "Dispatcher.h"
#include <sys/epoll.h>
#include "EventLoop.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "log.h"
//����Max,����Ŀ����
#define Max 520
//��ʼ��Epoll������
struct EpollData
{
	int epfd; //epoll�ĸ��ڵ�
	//��������
	struct epoll_event* events;
};
//epoll��ʼ��
static void* epollInit();
//epoll���
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
//epoll�Ƴ�ɾ��
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
//epoll �޸�
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
//epoll �¼����
static int epollDispatch(struct EventLoop* evLoop, int timeout);
//epoll �¼�����
static int epollClear(struct EventLoop* evLoop);
//��ɾ�Ĺ��в���
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop,int op); //op��Ӧ����ɾ��
//��ʼ��
// ʹ��alt ��ť����ͬʱ���static
// ����static�ֲ����������ڵ�ǰ����ʹ��
// ���ӵĽṹ��EpollDispatcher����ȫ�ֱ���
// �����Dispatcher�뵱ǰ��epollDispatcher��������
//�����ṹ��������Խṹ���������ָ������   
// ��EventLoop��ʹ��EpollDispatcher��Ҫ������������ʹ�ô�ȫ�ֱ���������exten
struct Dispatcher EpollDispatcher =
{
	epollInit,
	epollAdd,
	epollRemove,
	epollModify,
	epollDispatch,
	epollClear
};
static void* epollInit()
{
	struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
	data->epfd = epoll_create(10);
	if (data->epfd == -1)
	{
		perror("epoll create");
		exit(0);
	}
	//Ϊ�ṹ��ָ������ڴ棬���飬��������������������ɸ��ṹ���С�����ݿ�
	// calloc��malloc���򵥣����˷����ڴ棬��Ϊ��ʼ�����ڴ��ʼ��Ϊ0
	// Max�ڵ�����������ڵ��ڴ��С
	data->events = (struct epoll_event*)calloc(Max,sizeof(struct epoll_event));
	return data;   //�����뵽EventLoop��dispatcherData����
}
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd; //Ҫ�����ļ�������
	//����channel�Ķ�д�¼����Լ����壬��Ҫ�жϽ�������
	// ��λ�� ֻ�ж�Ӧ�Ķ�����11��Ϊ1����������0�������ReadEvent��Ӧ��ֵ
	int events = 0;
	if (channel->events & ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼�
		events |= EPOLLIN; //��EPOLLIN��Ҳ���������� ��λ����ͬΪ0 ��Ϊ1�� ��¼����ǰ���ͱ�������
	}
	if (channel->events & WriteEvent) //����else if,ĳЩʱ���д������
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	//�Ѵ�������channel���ļ���������ӵ�epoll���ϣ���Ҫ���channel�¼�
	// ���channel->fd��Ӧ��ʲô�¼� ����ļ���������ʲô�¼�

	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return ret;
}
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop) //epoll�ĸ��ڵ����evloop����
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
	if (ret == -1)
	{
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
}
// ����Ӵ�����ͬ
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
	if (ret == -1)
	{
		perror("epoll_ctl delete");
		exit(0);
	}
	// ͨ��channel�ͷŶ�Ӧ��TcpConnection��Դ
	// argΪ struct TcpConnection* conn = (struct TcpConnection*)arg;
	channel->destoryCallback(channel->arg);
	return ret;
}

static int epollModify(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
	if (ret == -1)
	{
		perror("epoll_ctl Modify");
		exit(0);
	}
	return ret;
}

static int epollDispatch(struct EventLoop* evLoop, int timeout)//��λ S ��ʱʱ��
{
	//��ʼ������evLoop����
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	int count = epoll_wait(data->epfd, data->events, Max, timeout * 1000); //����Ǻ���
	if (count == -1)
	{
		perror("epoll_wait");
	}
	// �����ļ�������
	for (int i = 0; i < count; ++i)
	{
		//��forѭ���а���Чѭ������ȡ��������Ӧ���¼����ļ���������������
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		//�ж��Ƿ�����쳣���������쳣ֱ�Ӵ�epoll����ɾ��
		// EPOLLERR�Զ˶Ͽ�����֮��EPOLLHUP�Զ˶Ͽ�������Ȼ�����ݲ����Ĵ���
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			//�Է��Ͽ������ӣ�ɾ��fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN)
		{
			//��
			eventActivate(evLoop, fd, ReadEvent);
		}
		if (events & EPOLLOUT)
		{
			//д
			eventActivate(evLoop, fd, WriteEvent);

		}
	}
	return 0;
}
static int epollClear(struct EventLoop* evLoop)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	//�ͷŵ�ַ
	free(data->events);  
	// �ر�epoll�ļ�������
	close(data->epfd);
	free(data);
	return 0;
}