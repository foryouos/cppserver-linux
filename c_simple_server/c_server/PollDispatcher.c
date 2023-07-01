#include "Dispatcher.h"
#include <poll.h>
#include "EventLoop.h"
#include <stdlib.h>
#include <stdio.h>
//定义Max,申请的块个数
#define Max 1024
//初始化Epoll的数据
struct PollData
{
	int maxfd;  //文件描述符最大值//maxfd主要控制右边的边界
	//传出参数
	struct pollfd fds[Max];
};
//epoll初始化
static void* pollInit();
//epoll添加
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop);
//epoll移除删除
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop);
//epoll 修改
static int pollModify(struct Channel* channel, struct EventLoop* evLoop);
//epoll 事件检测
static int pollDispatch(struct EventLoop* evLoop, int timeout);
//epoll 事件清理
static int pollClear(struct EventLoop* evLoop);
//初始化
// 使用alt 按钮向下同时添加static
// 加入static局部变量，仅在当前函数使用
// 不加的结构体EpollDispatcher属于全局变量
// 如何让Dispatcher与当前的epollDispatcher产生关联
//创建结构体变量，对结构体变量依次指定给它   
// 在EventLoop中使用EpollDispatcher，要在其它函数中使用此全局变量，加上exten
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
	//对应数组初始化
	for (int i = 0; i < Max; ++i)
	{
		data->fds[i].fd = -1; //初始化-1为无效的文件描述符
		data->fds[i].events = 0; //当前文件描述符检测的事件
		data->fds[i].revents = 0;  //返回的事件

	}

	return data;   //最后存入到EventLoop的dispatcherData里面
}
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop) //epoll的根节点存在evloop当中
{
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	int events = 0;
	if (channel->events & ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件
		events |= POLLIN; //将EPOLLIN（也是整形数） 按位或（相同为0 异为1） 记录当当前整型变量当中
	}
	if (channel->events & WriteEvent) //不用else if,某些时候读写都存在
	{
		events |= POLLOUT;
	}
	//找出空闲的可用元素
	int i;
	for (i = 0; i < Max; ++i)
	{
		if (data->fds[i].fd == -1)
		{
			//赋值操作
			data->fds[i].events = events;
			data->fds[i].fd = channel->fd;
			//更新data指向数组里maxfd，比较和最后一个数组里的下标 
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
// 与添加大致相同
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	int i;
	for (i = 0; i < Max; ++i)
	{
		if (data->fds[i].fd == channel->fd)
		{
			//赋值操作
			data->fds[i].events = 0;
			data->fds[i].revents = 0;
			data->fds[i].fd = -1;
			break;
		}
	}
	// 通过channel释放对应的TcpConnection资源
	// arg为 struct TcpConnection* conn = (struct TcpConnection*)arg;
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
	if (channel->events & ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件
		events |= POLLIN; //将EPOLLIN（也是整形数） 按位或（相同为0 异为1） 记录当当前整型变量当中
	}
	if (channel->events & WriteEvent) //不用else if,某些时候读写都存在
	{
		events |= POLLOUT;
	}
	//找出空闲的可用元素
	int i;
	for (i = 0; i < Max; ++i)
	{
		if (data->fds[i].fd == -1)
		{
			//赋值操作
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
//最核心
static int pollDispatch(struct EventLoop* evLoop, int timeout)//单位 S 超时时长
{
	//初始化数据evLoop数据
	struct PollData* data = (struct PollData*)evLoop->dispatcherData;
	//第二个参数 内存需要根据此参数确定遍历数组的时候的范围
	int count = poll(data->fds, data->maxfd+1, timeout * 1000); //最后是毫秒
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
			// 读
			eventActivate(evLoop, data->fds[i].fd, ReadEvent);
		}
		if (data->fds[i].revents & POLLOUT)
		{
			// 写
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