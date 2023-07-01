#include "Dispatcher.h"
#include <sys/epoll.h>
#include "EventLoop.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "log.h"
//定义Max,申请的块个数
#define Max 520
//初始化Epoll的数据
struct EpollData
{
	int epfd; //epoll的根节点
	//传出参数
	struct epoll_event* events;
};
//epoll初始化
static void* epollInit();
//epoll添加
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
//epoll移除删除
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
//epoll 修改
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
//epoll 事件检测
static int epollDispatch(struct EventLoop* evLoop, int timeout);
//epoll 事件清理
static int epollClear(struct EventLoop* evLoop);
//增删改公有部分
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop,int op); //op对应的添删改
//初始化
// 使用alt 按钮向下同时添加static
// 加入static局部变量，仅在当前函数使用
// 不加的结构体EpollDispatcher属于全局变量
// 如何让Dispatcher与当前的epollDispatcher产生关联
//创建结构体变量，对结构体变量依次指定给它   
// 在EventLoop中使用EpollDispatcher，要在其它函数中使用此全局变量，加上exten
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
	//为结构体指针分配内存，数组，根据数组的容量分配若干个结构体大小的数据块
	// calloc比malloc更简单，除了分配内存，还为初始化的内存初始化为0
	// Max节点个数，单个节点内存大小
	data->events = (struct epoll_event*)calloc(Max,sizeof(struct epoll_event));
	return data;   //最后存入到EventLoop的dispatcherData里面
}
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd; //要检查的文件描述符
	//由于channel的读写事件由自己定义，需要判断进行设置
	// 按位与 只有对应的二进制11才为1，若不等于0，则等于ReadEvent对应的值
	int events = 0;
	if (channel->events & ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件
		events |= EPOLLIN; //将EPOLLIN（也是整形数） 按位或（相同为0 异为1） 记录当当前整型变量当中
	}
	if (channel->events & WriteEvent) //不用else if,某些时候读写都存在
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	//把传过来的channel的文件描述符添加到epoll树上，需要检测channel事件
	// 检测channel->fd对应的什么事件 最后文件描述符的什么事件

	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return ret;
}
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop) //epoll的根节点存在evloop当中
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
	if (ret == -1)
	{
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
}
// 与添加大致相同
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
	if (ret == -1)
	{
		perror("epoll_ctl delete");
		exit(0);
	}
	// 通过channel释放对应的TcpConnection资源
	// arg为 struct TcpConnection* conn = (struct TcpConnection*)arg;
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

static int epollDispatch(struct EventLoop* evLoop, int timeout)//单位 S 超时时长
{
	//初始化数据evLoop数据
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	int count = epoll_wait(data->epfd, data->events, Max, timeout * 1000); //最后是毫秒
	if (count == -1)
	{
		perror("epoll_wait");
	}
	// 遍历文件描述符
	for (int i = 0; i < count; ++i)
	{
		//在for循环中把有效循环依次取出，将对应的事件和文件描述符保存下来
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		//判断是否出现异常，若出现异常直接从epoll树上删除
		// EPOLLERR对端断开连接之后，EPOLLHUP对端断开连接仍然发数据产生的错误
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			//对方断开了连接，删除fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN)
		{
			//读
			eventActivate(evLoop, fd, ReadEvent);
		}
		if (events & EPOLLOUT)
		{
			//写
			eventActivate(evLoop, fd, WriteEvent);

		}
	}
	return 0;
}
static int epollClear(struct EventLoop* evLoop)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	//释放地址
	free(data->events);  
	// 关闭epoll文件描述符
	close(data->epfd);
	free(data);
	return 0;
}