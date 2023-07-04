#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>

ThreadPool::ThreadPool(EventLoop* mainLoop, int count)
{
	m_index = 0;
	m_isStart = false; //默认是没有启动的
	m_mainLoop = mainLoop;
	m_threadNum = count;
	m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
	// 将子线程中的资源释放掉
	for (auto item : m_workerThreads)
	{
		delete item;
	}
}

void ThreadPool::Run()
{
	assert(!m_isStart); //运行期间此条件不能错
	//判断是不是主线程
	if(m_mainLoop->getTHreadID() != this_thread::get_id())
	{
		exit(0);
	}
	// 将线程池设置状态标志为启动
	m_isStart = true;
	// 子线程数量大于0
	if (m_threadNum > 0)
	{
		for (int i = 0; i < m_threadNum; ++i)
		{
			WorkerThread* subThread = new WorkerThread(i); // 调用子线程
			subThread->Run();
			m_workerThreads.push_back(subThread);
		}
	}
}

EventLoop* ThreadPool::takeWorkerEventLoop()
{
	//由主线程来调用线程池取出反应堆模型
	assert(m_isStart); //当前程序必须是运行的
	//判断是不是主线程
	if (m_mainLoop->getTHreadID() != this_thread::get_id())
	{
		exit(0);
	}
	//从线程池中找到一个子线层，然后取出里面的反应堆实例
	EventLoop* evLoop = m_mainLoop; //将主线程实例初始化
	if (m_threadNum > 0)
	{
		evLoop = m_workerThreads[m_index]->getEventLoop();
		//雨露均沾，不能一直是一个pool->index线程
		m_index = ++m_index % m_threadNum;
	}
	return evLoop;
}
