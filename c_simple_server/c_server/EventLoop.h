#pragma once
#include "Dispatcher.h"
#include <stdbool.h>
#include "Channel.h"
#include "ChannelMap.h"
#include <pthread.h>
//不同分发模型实例
extern struct Dispatcher EpollDispatcher;  
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;
//处理该节点中Channel
enum ELemType
{
	ADD,
	DELETE,
	MODIFY
};


//定义任务队列的节点 类型，文件描述符信息，和下一个指针
struct ChannelElement
{
	int type;                  //如何处理该节点中Channel
	struct Channel* channel;   //文件描述符
	struct ChannelElement* next;
};


struct Dispatcher;  //先定义出，避免编译器在相互包含情况下出现蛋生鸡鸡生蛋问题
// 在EventLoop存储，一个Dispatcher存储这对应的DispatcherData
// 头结点开关，锁，线程
struct EventLoop 
{
	//加入开关 EventLoop是否工作
	bool isQuit;
	struct Dispatcher* dispatcher;  // 分发模型dispatcher实例 select poll epoll，上面实例三选一
	void* dispatcherData;    //所需要的数据
	//任务队列，存储任务，遍历任务队列就可以修改dispatcher检测的文件描述符
	// 链表
	//头结点和为节点
	struct ChannelElement* head;
	struct ChannelElement* tail;

	//map 文件描述符和Channel之间的对应关系  通过数组实现
	struct ChannelMap* channelmap;
	// 线程相关，线程ID，name
	pthread_t threadID;
	char threadName[32];  //主线程只有一个，固定名称，初始化要分为两个
	//互斥锁，保护任务队列
	pthread_mutex_t mutex;
	// 整型数组
	int socketPair[2]; //存储本地通信fd通过socketpair初始化
};

//EventLoop 实例化
struct EventLoop* eventLoopInit();
//子线程带线程名称参数
struct EventLoop* eventLoopInitEx(const char* threadName);
//启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop);
// 启动之后就会出现一些文件描述符需要处理
// 处理激活的文件描述符fd,和激活的事件
int eventActivate(struct EventLoop* evLoop,int fd,int event);
// 添加任务到任务队列 ，添加任务队列可能存在同时访问，加互斥锁
int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel,int type);
// 处理任务队列中的人物
int eventLoopProcessTask(struct EventLoop* evLoop);

// 添加 处理dispatcher中的节点
// 把任务节点中的任务添加到dispatcher对应的检测集合里面
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
// 移除  处理dispatcher中的节点
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
// 修改 处理dispatcher中的节点
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
// 释放channel需要资源释放channel 关掉文件描述符，地址堆内存释放，channel和dispatcher的关系需要删除
int destoryChannel(struct EventLoop* evLoop, struct Channel* channel);