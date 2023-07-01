#include "TcpConnection.h"
#include "HttpRequest.h"
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
// 接受数据操作
int processRead(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// 接受数据 最后要存储到readBuf里面
	int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
	// data起始地址 readPos该读的地址位置
	Debug("接收到的http请求数据: %s", conn->readBuf->data + conn->readBuf->readPos);

	if (count > 0)
	{
		// 接受了http请求，解析http请求
		int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
		//添加检测写事件
		writeEventEnable(conn->channel, true);
		//  MODIFY修改检测读写事件
		eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif
		bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, socket);
		if (!flag)
		{
			//解析失败，回复一个简单的HTML
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendString(conn->writeBuf, errMsg);
		}
	}
	else
	{
#ifdef MSG_SEND_AUTO  //如果被定义，
		eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	}
	// 断开连接 完全写入缓存区再发送不能立即关闭，还没有发送
#ifndef MSG_SEND_AUTO  //如果没有被定义，
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	return 0;
}
//写回调函数，处理写事件，将writeBuf中的数据发送给客户端
int processWrite(void* arg)
{
	Debug("开始发送数据了(基于写事件发送)....");
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// 发送数据
	int count = bufferSendData(conn->writeBuf, conn->channel->fd);
	if (count > 0)
	{
		// 判断数据是否全部被发送出去
		if (bufferReadableSize(conn->writeBuf) == 0)
		{
			// 数据发送完毕
			// 1，不再检测写事件 --修改channel中保存的事件
			writeEventEnable(conn->channel, false);
			// 2, 修改dispatcher中检测的集合，往enentLoop反映模型认为队列节点标记为modify
			eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
			//3，若不通信，删除这个节点
			eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
		}
	}
	return 0;
}


struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evloop)
{
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	conn->evLoop = evloop;
	conn->readBuf = bufferInit(10240); //10K
	conn->writeBuf = bufferInit(10240);
	// http 初始化
	conn->request = httpRequestInit();
	conn->response = httpResponseInit();

	// 连接的名字
	sprintf(conn->name, "Connection-%d", fd);
	// 服务器最迫切想知道的时候，客户端有没有数据到达
	conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestory,conn);
	// 把channel放到任务循环的任务队列里面
	eventLoopAddTask(evloop, conn->channel, ADD);
	Debug("和客户端建立连接,threadName: %s,threadID:%s,connName:%s", evloop->threadName,evloop->threadID,conn->name);
	return conn;
}
// tcp断开连接时断开
int tcpConnectionDestory(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// 判断conn是否为空
	if (conn != NULL)
	{
		// 判断 读写缓存区是否还有数据没有被处理
		if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0
			&& conn->writeBuf && bufferReadableSize(conn->writeBuf) == 0)
		{
			destoryChannel(conn->evLoop, conn->channel);
			bufferDestory(conn->readBuf);
			bufferDestory(conn->writeBuf);
			httpRequetDestory(conn->request);
			httpResponseDestory(conn->response);
			free(conn);
		}
	}
	Debug("连接断开，释放资源, connName： %s", conn->name);
	return 0;
}
