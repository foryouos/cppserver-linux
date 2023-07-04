#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

// 服务器类 负责服务器初始化，设置监听，启动服务器以及接受主线程的请求连接
class TcpServer
{
public:
	TcpServer(unsigned short port, int threadNum);
	// 初始化监听
	void setListen();
	// 启动服务器 - 启动线程池-对监听的套接字进行封装，并放到主线程的任务队列，启动反应堆模型
	void Run();
	// 接受主线程的连接请求调用函数
	static int acceptConnection(void* arg);

private:
	int m_threadNum;  // 存储构造函数传入的子线程个数
	EventLoop* m_mainLoop;  // 指针指向主线程拥有的主线程反映实例 
	ThreadPool* m_threadPool;  // 指针保存主线程的线程池
	int m_lfd;            //服务器端用于监听的文件描述符
	unsigned short m_port; //端口
};
