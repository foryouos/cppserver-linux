#include "Dispatcher.h"
#include <poll.h>
#include "EventLoop.h"
#include <stdlib.h>
#include <stdio.h>
//����Max,����Ŀ����
#define Max 1024
//��ʼ��Epoll������
struct PollData
{
	int maxfd;  //�ļ����������ֵ//maxfd��Ҫ�����ұߵı߽�
	//��������
	struct pollfd fds[Max];
};
//epoll��ʼ��
static void* pollInit();
//epoll���
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop);
//epoll�Ƴ�ɾ��
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop);
//epoll �޸�
static int pollModify(struct Channel* channel, struct EventLoop* evLoop);
//epoll �¼����
static int pollDispatch(struct EventLoop* evLoop, int timeout);
//epoll �¼�����
static int pollClear(struct EventLoop* evLoop);
//��ʼ��
// ʹ��alt ��ť����ͬʱ���static
// ����static�ֲ����������ڵ�ǰ����ʹ��
// ���ӵĽṹ��EpollDispatcher����ȫ�ֱ���
// �����Dispatcher�뵱ǰ��epollDispatcher��������
//�����ṹ��������Խṹ���������ָ������   
// ��EventLoop��ʹ��EpollDispatcher��Ҫ������������ʹ�ô�ȫ�ֱ���������exten
struct Dispatcher PollDispatcher =
{
	pollInit,
	pollAdd,
	pollRemove,
	pollModify,
	pollDispatch,
	pollClear
};
static void* pollInit()
{
	struct PollData* data = (struct PollData*)malloc(sizeof(struct PollData));
	data->maxfd = 0;
	//��Ӧ�����ʼ��
	for (int i = 0; i < Max; ++i)
	{
		data->fds[i].fd = -1; //��ʼ��-1Ϊ��Ч���ļ�������
		data->fds[i].events = 0; //��ǰ�ļ������������¼�
		data->fds[i].revents = 0;  //���ص��¼�

	}

	return data;   //�����뵽EventLoop��dispatcherData����
}
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop) //epoll�ĸ��ڵ����evloop����
{
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	int events = 0;
	if (channel->events & ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼�
		events |= POLLIN; //��EPOLLIN��Ҳ���������� ��λ����ͬΪ0 ��Ϊ1�� ��¼����ǰ���ͱ�������
	}
	if (channel->events & WriteEvent) //����else if,ĳЩʱ���д������
	{
		events |= POLLOUT;
	}
	//�ҳ����еĿ���Ԫ��
	int i;
	for (i = 0; i < Max; ++i)
	{
		if (data->fds[i].fd == -1)
		{
			//��ֵ����
			data->fds[i].events = events;
			data->fds[i].fd = channel->fd;
			//����dataָ��������maxfd���ȽϺ����һ����������±� 
			data->maxfd = i > data->maxfd  ? i : data->maxfd;
			break;
		}
	}
	if (i >= Max)
	{
		return -1;
	}

	return 0;
}
// ����Ӵ�����ͬ
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	int i;
	for (i = 0; i < Max; ++i)
	{
		if (data->fds[i].fd == channel->fd)
		{
			//��ֵ����
			data->fds[i].events = 0;
			data->fds[i].revents = 0;
			data->fds[i].fd = -1;
			break;
		}
	}
	// ͨ��channel�ͷŶ�Ӧ��TcpConnection��Դ
	// argΪ struct TcpConnection* conn = (struct TcpConnection*)arg;
	channel->destoryCallback(channel->arg); 
	if (i >= Max)
	{
		return -1;
	}

	return 0;
}

static int pollModify(struct Channel* channel, struct EventLoop* evLoop)
{
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	int events = 0;
	if (channel->events & ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼�
		events |= POLLIN; //��EPOLLIN��Ҳ���������� ��λ����ͬΪ0 ��Ϊ1�� ��¼����ǰ���ͱ�������
	}
	if (channel->events & WriteEvent) //����else if,ĳЩʱ���д������
	{
		events |= POLLOUT;
	}
	//�ҳ����еĿ���Ԫ��
	int i;
	for (i = 0; i < Max; ++i)
	{
		if (data->fds[i].fd == -1)
		{
			//��ֵ����
			data->fds[i].events = events;
			break;
		}
	}
	if (i >= Max)
	{
		return -1;
	}

	return 0;
}
//�����
static int pollDispatch(struct EventLoop* evLoop, int timeout)//��λ S ��ʱʱ��
{
	//��ʼ������evLoop����
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	//�ڶ������� �ڴ���Ҫ���ݴ˲���ȷ�����������ʱ��ķ�Χ
	int count = poll(data->fds, data->maxfd+1, timeout * 1000); //����Ǻ���
	if (count == -1)
	{
		perror("poll");
		exit(0);
	}
	for (int i = 0; i <= data->maxfd; ++i)
	{
		if (data->fds->fd == -1)
		{
			continue;
		}

		if (data->fds[i].revents & POLLIN)
		{
			// ��
			eventActivate(evLoop, data->fds[i].fd, ReadEvent);
		}
		if (data->fds[i].revents & POLLOUT)
		{
			// д
			eventActivate(evLoop, data->fds[i].fd, WriteEvent);
		}
	}
	return 0;
}
static int pollClear(struct EventLoop* evLoop)
{
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	free(data);
	return 0;
}