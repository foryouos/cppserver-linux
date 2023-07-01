#pragma once
#include <pthread.h>
#include "EventLoop.h"
// 工作线程

// 定义子线程结构体
struct WorkerThread
{
	pthread_t threadID; //ID
	char name[24];      //线程名字
	pthread_mutex_t mutex; //线程阻塞
	pthread_cond_t cond;   //条件变量
	struct EventLoop* evLoop; //反映堆模型 ，线程执行什么任务，取决于往反应堆模型添加了什么数据
};

//初始化
 // 线程地址，index是当前线程的第几个 拼接成子线程地址
int workerThreadInit(struct WorkerThread* thread, int index);
// 线程启动,传入线程
void workerThreadRun(struct WorkerThread* thread);
