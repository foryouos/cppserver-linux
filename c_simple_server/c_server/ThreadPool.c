#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)
{
	//Ϊ�̳߳�����һ����ڴ�
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	pool->index = 0;
	pool->isStart = false; //Ĭ����û��������
	pool->mainLoop = mainLoop;
	pool->threadNum = count;
	pool->workerThreads = (struct ThreadPool*)malloc(sizeof(struct WorkerThread) * count);
	return pool;
}

void threadPoolRun(struct ThreadPool* pool)
{
	assert(pool && !pool->isStart); //�����ڼ���������ܴ�
	//�ж��ǲ������߳�
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	// ���̳߳�����״̬��־Ϊ����
	pool->isStart = true;
	// �����й����̳߳�ʼ�������������߳�����
	if (pool->threadNum)
	{
		for (int i = 0; i < pool->threadNum; ++i)
		{
			workerThreadInit(&pool->workerThreads[i],i);  //�����̳߳�ʼ��
			workerThreadRun(&pool->workerThreads[i]);     // ����
		}
	}

}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
	//�����߳��������̳߳�ȡ����Ӧ��ģ��
	assert(pool->isStart); //��ǰ������������е�
	//�ж��ǲ������߳�
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	//���̳߳����ҵ�һ�����߲㣬Ȼ��ȡ������ķ�Ӧ��ʵ��
	struct EventLoop* evLoop = pool->mainLoop; //�����߳�ʵ����ʼ��
	if (pool->threadNum > 0)
	{
		evLoop = pool->workerThreads[pool->index].evLoop;
		//��¶��մ������һֱ��һ��pool->index�߳�
		pool->index = ++pool->index % pool->threadNum;
	}
	return evLoop;
}
