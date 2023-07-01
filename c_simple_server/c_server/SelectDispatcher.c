#include "Dispatcher.h"
#include <sys/select.h>
#include "EventLoop.h"
#include <stdio.h>
#include <stdlib.h>
//定义Max,申请的块个数
#define Max 1024
//初始select的数据
struct SelectData
{
	fd_set readSet;
	fd_set writeSet;
};
//初始化
static void* selectInit();
//添加
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop);
//移除删除
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop);
// 修改
static int selectModify(struct Channel* channel, struct EventLoop* evLoop);
//事件检测
static int selectDispatch(struct EventLoop* evLoop, int timeout);
// 事件清理
static int selectClear(struct EventLoop* evLoop);

//设置文件描述符集合
static void setFdSet(struct Channel* channel, struct SelectData* data);
//清空文件描述符集合
static void clearFdset(struct Channel* channel, struct SelectData* data);

//初始化
// 使用alt 按钮向下同时添加static
// 加入static局部变量，仅在当前函数使用
// 不加的结构体EpollDispatcher属于全局变量
// 如何让Dispatcher与当前的epollDispatcher产生关联
//创建结构体变量，对结构体变量依次指定给它   
// 在EventLoop中使用EpollDispatcher，要在其它函数中使用此全局变量，加上exten
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
	//对集合进行清空，所有标志位设置为0
	FD_ZERO(&data->readSet);
	FD_ZERO(&data->writeSet);  //文件描述符的地址
	return data;   //最后存入到EventLoop的dispatcherData里面
}


//设置文件描述符集合
static void setFdSet(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件 
		// 添加到对应的读写事件内
		FD_SET(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent) //不用else if,某些时候读写都存在
	{
		FD_SET(channel->fd, &data->writeSet);
	}
}
//清空文件描述符集合
static void clearFdset(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent) //ReadEvent自定义，按位与
	{
		// 清除相应的读事件 
		FD_CLR(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent) //不用else if,某些时候读写都存在
	{
		FD_CLR(channel->fd, &data->writeSet);
	}
}


static int selectAdd(struct Channel* channel, struct EventLoop* evLoop) //epoll的根节点存在evloop当中
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	if (channel->fd >= Max) //select的最大文件描述符是1024
	{
		return -1;
	}
	
	setFdSet(channel, data);

	return 0;
}
// 与添加大致相同
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	clearFdset(channel, data);
	// 通过channel释放对应的TcpConnection资源
	// arg为 struct TcpConnection* conn = (struct TcpConnection*)arg;
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
//最核心
static int selectDispatch(struct EventLoop* evLoop, int timeout)//单位 S 超时时长
{
	//初始化数据evLoop数据
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	//超时时长
	struct timeval val;
	val.tv_sec = timeout; //秒
	val.tv_usec = 0; //微秒,用不到也得初始化为0，系统会相加，否则会随机数

	// 234传入传出参数，内核会修改传入数据，原始数据传给内核之后，数据越来越少，出现丢失现象，
	// 对原始数据进行备份
	fd_set rdtmp = data->readSet;
	fd_set wrtmp = data->writeSet;

	int count = select(Max,&rdtmp,&wrtmp,NULL,&val); //最后是毫秒
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	// 遍历判断那个文件描述符被激活了
	for (int i = 0; i < Max; ++i)
	{
		if (FD_ISSET(i,&rdtmp)) //读被激活了
		{
			// 读
			eventActivate(evLoop, i, ReadEvent);
		}

		if (FD_ISSET(i, &wrtmp))
		{
			// 写
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