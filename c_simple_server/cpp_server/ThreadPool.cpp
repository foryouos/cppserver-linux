#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>

ThreadPool::ThreadPool(EventLoop* mainLoop, int count)
{
	m_index = 0;
	m_isStart = false; //Ĭ����û��������
	m_mainLoop = mainLoop;
	m_threadNum = count;
	m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
	// �����߳��е���Դ�ͷŵ�
	for (auto item : m_workerThreads)
	{
		delete item;
	}
}

void ThreadPool::Run()
{
	assert(!m_isStart); //�����ڼ���������ܴ�
	//�ж��ǲ������߳�
	if(m_mainLoop->getTHreadID() != this_thread::get_id())
	{
		exit(0);
	}
	// ���̳߳�����״̬��־Ϊ����
	m_isStart = true;
	// ���߳���������0
	if (m_threadNum > 0)
	{
		for (int i = 0; i < m_threadNum; ++i)
		{
			WorkerThread* subThread = new WorkerThread(i); // �������߳�
			subThread->Run();
			m_workerThreads.push_back(subThread);
		}
	}
}

EventLoop* ThreadPool::takeWorkerEventLoop()
{
	//�����߳��������̳߳�ȡ����Ӧ��ģ��
	assert(m_isStart); //��ǰ������������е�
	//�ж��ǲ������߳�
	if (m_mainLoop->getTHreadID() != this_thread::get_id())
	{
		exit(0);
	}
	//���̳߳����ҵ�һ�����߲㣬Ȼ��ȡ������ķ�Ӧ��ʵ��
	EventLoop* evLoop = m_mainLoop; //�����߳�ʵ����ʼ��
	if (m_threadNum > 0)
	{
		evLoop = m_workerThreads[m_index]->getEventLoop();
		//��¶��մ������һֱ��һ��pool->index�߳�
		m_index = ++m_index % m_threadNum;
	}
	return evLoop;
}
