#include "Dispatcher.h"
#include <sys/select.h>
#include "EventLoop.h"
#include <stdio.h>
#include <stdlib.h>
//����Max,����Ŀ����
#define Max 1024
//��ʼselect������
struct SelectData
{
	fd_set readSet;
	fd_set writeSet;
};
//��ʼ��
static void* selectInit();
//���
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop);
//�Ƴ�ɾ��
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop);
// �޸�
static int selectModify(struct Channel* channel, struct EventLoop* evLoop);
//�¼����
static int selectDispatch(struct EventLoop* evLoop, int timeout);
// �¼�����
static int selectClear(struct EventLoop* evLoop);

//�����ļ�����������
static void setFdSet(struct Channel* channel, struct SelectData* data);
//����ļ�����������
static void clearFdset(struct Channel* channel, struct SelectData* data);

//��ʼ��
// ʹ��alt ��ť����ͬʱ���static
// ����static�ֲ����������ڵ�ǰ����ʹ��
// ���ӵĽṹ��EpollDispatcher����ȫ�ֱ���
// �����Dispatcher�뵱ǰ��epollDispatcher��������
//�����ṹ��������Խṹ���������ָ������   
// ��EventLoop��ʹ��EpollDispatcher��Ҫ������������ʹ�ô�ȫ�ֱ���������exten
struct Dispatcher SelectDispatcher =
{
	selectInit,
	selectAdd,
	selectRemove,
	selectModify,
	selectDispatch,
	selectClear
};
static void* selectInit()
{
	struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
	//�Լ��Ͻ�����գ����б�־λ����Ϊ0
	FD_ZERO(&data->readSet);
	FD_ZERO(&data->writeSet);  //�ļ��������ĵ�ַ
	return data;   //�����뵽EventLoop��dispatcherData����
}


//�����ļ�����������
static void setFdSet(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼� 
		// ��ӵ���Ӧ�Ķ�д�¼���
		FD_SET(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent) //����else if,ĳЩʱ���д������
	{
		FD_SET(channel->fd, &data->writeSet);
	}
}
//����ļ�����������
static void clearFdset(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		// �����Ӧ�Ķ��¼� 
		FD_CLR(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent) //����else if,ĳЩʱ���д������
	{
		FD_CLR(channel->fd, &data->writeSet);
	}
}


static int selectAdd(struct Channel* channel, struct EventLoop* evLoop) //epoll�ĸ��ڵ����evloop����
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	if (channel->fd >= Max) //select������ļ���������1024
	{
		return -1;
	}
	
	setFdSet(channel, data);

	return 0;
}
// ����Ӵ�����ͬ
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	clearFdset(channel, data);
	// ͨ��channel�ͷŶ�Ӧ��TcpConnection��Դ
	// argΪ struct TcpConnection* conn = (struct TcpConnection*)arg;
	channel->destoryCallback(channel->arg);
	return 0;
}

static int selectModify(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	
	setFdSet(channel, data);
	clearFdset(channel, data);

	return 0;
}
//�����
static int selectDispatch(struct EventLoop* evLoop, int timeout)//��λ S ��ʱʱ��
{
	//��ʼ������evLoop����
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	//��ʱʱ��
	struct timeval val;
	val.tv_sec = timeout; //��
	val.tv_usec = 0; //΢��,�ò���Ҳ�ó�ʼ��Ϊ0��ϵͳ����ӣ�����������

	// 234���봫���������ں˻��޸Ĵ������ݣ�ԭʼ���ݴ����ں�֮������Խ��Խ�٣����ֶ�ʧ����
	// ��ԭʼ���ݽ��б���
	fd_set rdtmp = data->readSet;
	fd_set wrtmp = data->writeSet;

	int count = select(Max,&rdtmp,&wrtmp,NULL,&val); //����Ǻ���
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	// �����ж��Ǹ��ļ���������������
	for (int i = 0; i < Max; ++i)
	{
		if (FD_ISSET(i,&rdtmp)) //����������
		{
			// ��
			eventActivate(evLoop, i, ReadEvent);
		}

		if (FD_ISSET(i, &wrtmp))
		{
			// д
			eventActivate(evLoop, i, WriteEvent);
		}
		
	}
	return 0;
}
static int selectClear(struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	free(data);
	return 0;
}