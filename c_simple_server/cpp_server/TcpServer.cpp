#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
TcpServer::TcpServer(unsigned short port, int threadNum)
{
	m_port = port;
	m_mainLoop = new EventLoop;
	m_threadNum = threadNum;
	m_threadPool = new ThreadPool(m_mainLoop, threadNum);
	setListen();
}

void TcpServer::setListen()
{
	//1，创建监听的fd
	// AF_INET 基于IPV4，0 流式协议中的TCP协议
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_lfd == -1)
	{
		perror("socket"); //创建失败
		return ;
	}
	//2，设置端口复用
	int opt = 1; //1端口复用
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt"); //创建失败
		return ;
	}
	//3，绑定端口
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //IPV4协议
	addr.sin_port = htons(m_port); //将端口转为网络字节序 2的16次方65536个最大
	addr.sin_addr.s_addr = INADDR_ANY; //本地所有IP地址
	ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind"); //失败
		return ;
	}
	//4，设置监听  一次性可以和多少客户端连接,内核最大128，若给很大，内核会改为128
	ret = listen(m_lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return ;
	}
}

void TcpServer::Run()
{
	Debug("服务器已经启动....");
	// 启动线程池
	m_threadPool->Run();
	// 添加检测的任务
	// 初始化channel实例
	Channel* channel = new Channel(m_lfd,FDEvent::ReadEvent,
		acceptConnection, nullptr, nullptr, this); // TODO //只要服务器还在运行监听就不会停止，
	//将当前的channel添加到任务队列里面
	m_mainLoop->AddTask(channel, ElemType::ADD);
	// 启动反应堆模型，开始工作
	m_mainLoop->Run();  //启动反应堆模型
}
int TcpServer::acceptConnection(void* arg)
{
	TcpServer* server = static_cast<TcpServer*>(arg);
	// 和客户端建立连接
	int cfd = accept(server->m_lfd, NULL, NULL);
	// 取出子线程反应堆实例，处理cfd
	EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	// 将cfd放到TcpConnection处理 evLoop放到子线程 建立连接在主线程，和客户端通信都在子线程中运行
	// 匿名对象
	new TcpConnection(cfd, evLoop);
	return 0;
}