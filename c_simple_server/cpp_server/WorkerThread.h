#pragma once
#include "EventLoop.h"
#include <mutex>
#include <thread>
#include <condition_variable>
using namespace std;
// 工作线程，线程启动，获取EventLoop
class WorkerThread
{
public:
	WorkerThread(int index);
	~WorkerThread();
	
	// 线程启动,传入线程
	void Run();
	// 
	inline EventLoop* getEventLoop()
	{
		return m_evLoop;
	}
private:
	void* Running();
private:
	//保存线程地址指针
	thread* m_thread;
	thread::id m_threadID; //ID
	string m_name;      //线程名字
	mutex m_mutex; //线程阻塞
	condition_variable m_cond;   //条件变量
	EventLoop* m_evLoop; //反映堆模型 ，线程执行什么任务，取决于往反应堆模型添加了什么数据
};


