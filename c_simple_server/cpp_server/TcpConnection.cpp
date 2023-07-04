#include "TcpConnection.h"
#include "log.h"

TcpConnection::TcpConnection(int fd, EventLoop* evloop)
{
	//并没有创建evloop，当前的TcpConnect都是在子线程中完成的
	m_evLoop = evloop;
	m_readBuf = new Buffer(10240); //10K
	m_writeBuf = new Buffer(10240);
	// 初始化
	m_request = new HttpRequest;
	m_response = new HttpResponse;

	m_name = "Connection-" + to_string(fd);

	// 服务器最迫切想知道的时候，客户端有没有数据到达
	m_channel =new Channel(fd,FDEvent::ReadEvent, processRead, processWrite, destory, this);
	// 把channel放到任务循环的任务队列里面
	evloop->AddTask(m_channel, ElemType::ADD);
}
// 接受数据操作
TcpConnection::~TcpConnection()
{
	// 判断 读写缓存区是否还有数据没有被处理
	if (m_readBuf && m_readBuf->readableSize() == 0
		&& m_writeBuf && m_writeBuf->readableSize() == 0)
	{
		delete m_writeBuf;
		delete m_readBuf;
		delete m_request;
		delete m_response;
		m_evLoop->freeChannel(m_channel);
	}

	Debug("连接断开，释放资源, connName： %s", m_name);
}

int TcpConnection::processRead(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 接受数据 最后要存储到readBuf里面
	int socket = conn->m_channel->getSocket();
	int count = conn->m_readBuf->socketRead(socket);
	// data起始地址 readPos该读的地址位置
	Debug("接收到的http请求数据: %s", conn->m_readBuf->data());

	if (count > 0)
	{
		// 接受了http请求，解析http请求
		
#ifdef MSG_SEND_AUTO
		//添加检测写事件
		conn->m_channel->writeEventEnable(true);
		//  MODIFY修改检测读写事件
		conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
#endif
		bool flag = conn->m_request->parseHttpRequest(
			conn->m_readBuf, conn->m_response,
			conn->m_writeBuf, socket);
		if (!flag)
		{
			//解析失败，回复一个简单的HTML
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else
	{
		
#ifdef MSG_SEND_AUTO  //如果被定义，
		//断开连接
		conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	}
	// 断开连接 完全写入缓存区再发送不能立即关闭，还没有发送
#ifndef MSG_SEND_AUTO  //如果没有被定义，
	conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}

//写回调函数，处理写事件，将writeBuf中的数据发送给客户端
int TcpConnection::processWrite(void* arg)
{
	Debug("开始发送数据了(基于写事件发送)....");
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 发送数据
	int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0)
	{
		// 判断数据是否全部被发送出去
		if (conn->m_writeBuf->readableSize() == 0)
		{
			// 数据发送完毕
			// 1，不再检测写事件 --修改channel中保存的事件
			conn->m_channel->writeEventEnable(false);
			// 2, 修改dispatcher中检测的集合，往enentLoop反映模型认为队列节点标记为modify
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
			//3，若不通信，删除这个节点
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}

// tcp断开连接时断开
int TcpConnection::destory(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	if (conn != nullptr)
	{
		delete conn;
	}
	return 0;
}
