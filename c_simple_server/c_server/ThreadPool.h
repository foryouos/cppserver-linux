#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include "WorkerThread.h"
#include <stdio.h>
//�����̳߳�
struct ThreadPool
{
	//���̵߳ķ�Ӧ��ģ��
	struct EventLoop* mainLoop; //��Ҫ�������ݣ�����Ϳͻ��˽���������һ������
	bool isStart;				// �ж��̳߳��Ƿ�������
	int threadNum;				// ���߳�����
	struct WorkerThread* workerThreads; // �������߳����飬�����ж�̬�Ĵ���������threadNum��̬����ռ�
	int index;					// ��ǰ���߳�
};

//��ʼ���̳߳� mainLoop ���̷߳�����ʵ�� count��ӦthreadNum
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count);
// �̳߳�����
void threadPoolRun(struct ThreadPool *pool);
// ȡ���̳߳��е�ĳ�����߲�ķ�Ӧ��ʵ��
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);