#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)
{
	//为线程池申请一块堆内存
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	pool->index = 0;
	pool->isStart = false; //默认是没有启动的
	pool->mainLoop = mainLoop;
	pool->threadNum = count;
	pool->workerThreads = (struct ThreadPool*)malloc(sizeof(struct WorkerThread) * count);
	return pool;
}

void threadPoolRun(struct ThreadPool* pool)
{
	assert(pool && !pool->isStart); //运行期间此条件不能错
	//判断是不是主线程
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	// 将线程池设置状态标志为启动
	pool->isStart = true;
	// 将所有工作线程初始化，并将工作线程运行
	if (pool->threadNum)
	{
		for (int i = 0; i < pool->threadNum; ++i)
		{
			workerThreadInit(&pool->workerThreads[i],i);  //工作线程初始化
			workerThreadRun(&pool->workerThreads[i]);     // 运行
		}
	}

}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
	//由主线程来调用线程池取出反应堆模型
	assert(pool->isStart); //当前程序必须是运行的
	//判断是不是主线程
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	//从线程池中找到一个子线层，然后取出里面的反应堆实例
	struct EventLoop* evLoop = pool->mainLoop; //将主线程实例初始化
	if (pool->threadNum > 0)
	{
		evLoop = pool->workerThreads[pool->index].evLoop;
		//雨露均沾，不能一直是一个pool->index线程
		pool->index = ++pool->index % pool->threadNum;
	}
	return evLoop;
}
