#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
	struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcp->listener = listenerInit(port);
	tcp->mainLoop = eventLoopInit();
	tcp->threadNum = threadNum;
	tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
	return tcp;
}

struct Listener* listenerInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	//1，创建监听的fd
	// AF_INET 基于IPV4，0 流式协议中的TCP协议
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket"); //创建失败
		return NULL;
	}
	//2，设置端口复用
	int opt = 1; //1端口复用
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt"); //创建失败
		return NULL;
	}
	//3，绑定端口
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //IPV4协议
	addr.sin_port = htons(port); //将端口转为网络字节序 2的16次方65536个最大
	addr.sin_addr.s_addr = INADDR_ANY; //本地所有IP地址
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind"); //失败
		return NULL;
	}
	//4，设置监听  一次性可以和多少客户端连接,内核最大128，若给很大，内核会改为128
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return NULL;
	}
	//返回fd
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}
int acceptConnection(void* arg)
{
	struct TcpServer* server = (struct TcpServer*)arg;
	// 和客户端建立连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	// 取出子线程反应堆实例，处理cfd
	struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
	// 将cfd放到TcpConnection处理 evLoop放到子线程 建立连接在主线程，和客户端通信都在子线程中运行
	tcpConnectionInit(cfd, evLoop);
	return 0;
}
void tcpServerRun(struct TcpServer* server)
{
	Debug("服务器已经启动....");	
	// 启动线程池
	threadPoolRun(server->threadPool);
	
	// 添加检测的任务
	// 初始化channel实例
	struct Channel* channel = channelInit(server->listener->lfd, ReadEvent,
		acceptConnection, NULL,NULL,server);
	eventLoopAddTask(server->mainLoop, channel, ADD); //将当前的channel添加到任务队列里面

	// 启动反应堆模型，开始工作
	eventLoopRun(server->mainLoop);  //启动反应堆模型
	

}
