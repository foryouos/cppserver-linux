#pragma once
#include "EventLoop.h"
#include <mutex>
#include <thread>
#include <condition_variable>
using namespace std;
// �����̣߳��߳���������ȡEventLoop
class WorkerThread
{
public:
	WorkerThread(int index);
	~WorkerThread();
	
	// �߳�����,�����߳�
	void Run();
	// 
	inline EventLoop* getEventLoop()
	{
		return m_evLoop;
	}
private:
	void* Running();
private:
	//�����̵߳�ַָ��
	thread* m_thread;
	thread::id m_threadID; //ID
	string m_name;      //�߳�����
	mutex m_mutex; //�߳�����
	condition_variable m_cond;   //��������
	EventLoop* m_evLoop; //��ӳ��ģ�� ���߳�ִ��ʲô����ȡ��������Ӧ��ģ�������ʲô����
};


