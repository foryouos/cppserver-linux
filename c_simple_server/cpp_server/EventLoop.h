#pragma once
#include "Dispatcher.h"
#include <thread>
#include "Channel.h"
#include <queue>   //队列
#include <map>     //键值
#include <mutex>   //互斥锁
using namespace std;
//处理该节点中Channel事件类型
enum class ElemType:char
{
	ADD,
	DELETE,
	MODIFY
};
//定义任务队列的节点 类型，文件描述符信息
struct ChannelElement
{
	ElemType type;       //如何处理该节点中Channel
	Channel* channel;   //文件描述符信息
};
class Dispatcher;  //先定义出，避免编译器在相互包含情况下出现蛋生鸡鸡生蛋问题

// 处理所有的事件，启动反应堆模型，处理机会文件描述符后的事件,添加任务，处理任务队列
// 调用dispatcher中的添加移除，修改操作
// 存储着任务队列m_taskQ  存储fd和对应channel对应关系:m_channelmap
// 全局主线程->同时传入子线程
class EventLoop 
{
public:
	EventLoop();
	EventLoop(const string threadName);
	~EventLoop();
	//启动反应堆模型
	int Run();
	// 启动之后就会出现一些文件描述符需要处理
	// 处理激活的文件描述符fd,和激活的事件
	int eventActive(int fd, int event);
	// 添加任务到任务队列 ，添加任务队列可能存在同时访问，加互斥锁
	int AddTask(Channel* channel, ElemType type);
	// 处理任务队列中的人物
	int ProcessTaskQ();
	// 添加 处理dispatcher中的节点
	// 把任务节点中的任务添加到dispatcher对应的检测集合里面
	int Add(Channel* channel);
	// 移除  处理dispatcher中的节点
	int Remove(Channel* channel);
	// 修改 处理dispatcher中的节点
	int Modify(Channel* channel);
	// 释放channel需要资源释放channel 关掉文件描述符，地址堆内存释放，channel和dispatcher的关系需要删除
	int freeChannel(Channel* channel);
	static int readlocalMessage(void* arg);
	int readMessage();
	// 返回线程ID
	inline thread::id getTHreadID()
	{
		return m_threadID;
	}
	inline string getThreadName()
	{
		return m_threadName;
	}
private:
	void taskWakeup();
private:
	//加入开关 EventLoop是否工作
	bool m_isQuit;
	//该指针指向之类的实例epoll,poll,select
	Dispatcher* m_dispatcher; 
	//任务队列，存储任务，遍历任务队列就可以修改dispatcher检测的文件描述符
	//任务队列
	queue<ChannelElement*>m_taskQ;

	//map 文件描述符和Channel之间的对应关系  通过数组实现
	map<int,Channel*> m_channelmap;
	// 线程相关，线程ID，name
	thread::id m_threadID;
	string m_threadName;  //主线程只有一个，固定名称，初始化要分为两个
	//互斥锁，保护任务队列
	mutex m_mutex;
	// 整型数组
	int m_socketPair[2]; //存储本地通信fd通过socketpair初始化
};

