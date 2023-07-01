#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

struct Listener
{
	int lfd;            //监听
	unsigned short port; //端口
};
// 服务器结构体
struct TcpServer
{
	int threadNum;  //线程个数
	struct EventLoop* mainLoop;   
	struct ThreadPool* threadPool; 
	struct Listener* listener;
};
// 初始化
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
// 初始化监听
struct Listener*listenerInit(unsigned short port);
// 启动服务器 - 启动线程池-对监听的套接字进行封装，并放到主线程的任务队列，启动反应堆模型
void tcpServerRun(struct TcpServer* server);