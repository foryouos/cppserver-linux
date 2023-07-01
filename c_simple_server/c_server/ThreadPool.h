#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include "WorkerThread.h"
#include <stdio.h>
//定义线程池
struct ThreadPool
{
	//主线程的反应堆模型
	struct EventLoop* mainLoop; //主要用作备份，负责和客户端建立连接这一件事情
	bool isStart;				// 判断线程池是否启动啦
	int threadNum;				// 子线程数量
	struct WorkerThread* workerThreads; // 工作的线程数组，工作中动态的创建，根据threadNum动态分配空间
	int index;					// 当前的线程
};

//初始化线程池 mainLoop 主线程返工堆实例 count对应threadNum
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count);
// 线程池运行
void threadPoolRun(struct ThreadPool *pool);
// 取出线程池中的某个子线层的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);