#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <vector>
using namespace std;
//�����̳߳� �����̳߳أ�public����ȡ���̳߳���ĳ�����̵߳ķ�Ӧ��ʵ��EventLoop
class ThreadPool
{
public:
	ThreadPool(EventLoop* mainLoop, int count);
	~ThreadPool();
	// �̳߳�����
	void Run();
	// ȡ���̳߳��е�ĳ�����߲�ķ�Ӧ��ʵ��
	EventLoop* takeWorkerEventLoop();
private:
	//���̵߳ķ�Ӧ��ģ��
	EventLoop* m_mainLoop; //��Ҫ�������ݣ�����Ϳͻ��˽���������һ������
	bool m_isStart;				// �ж��̳߳��Ƿ�������
	int m_threadNum;				// ���߳�����
	vector<WorkerThread*> m_workerThreads; // �������߳����飬�����ж�̬�Ĵ���������threadNum��̬����ռ�
	int m_index;					// ��ǰ���߳�
};