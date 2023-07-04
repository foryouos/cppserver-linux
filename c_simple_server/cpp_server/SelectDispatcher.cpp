#include "Dispatcher.h"
#include <sys/select.h>
#include "EventLoop.h"
#include <stdio.h>
#include <stdlib.h>
#include "SelectDispatcher.h"
SelectDispatcher::SelectDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	//对集合进行清空，所有标志位设置为0
	FD_ZERO(&m_readSet);
	FD_ZERO(&m_writeSet);  //文件描述符的地址
	m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
	if (m_channel->getSocket() >= m_maxSize) //select的最大文件描述符是1024
	{
		return -1;
	}
	setFdSet();
	return 0;
}

int SelectDispatcher::remove()
{
	clearFdset();
	// 通过channel释放对应的TcpConnection资源
	// arg为 struct TcpConnection* conn = (struct TcpConnection*)arg;
	m_channel->destoryCallback(const_cast<void*>(m_channel->getArg()));
	return 0;
}

int SelectDispatcher::modify()
{

	setFdSet();
	clearFdset();
	return 0;
}

int SelectDispatcher::dispatch(int timeout)
{
	//超时时长
	struct timeval val;
	val.tv_sec = timeout; //秒
	val.tv_usec = 0; //微秒,用不到也得初始化为0，系统会相加，否则会随机数

	// 234传入传出参数，内核会修改传入数据，原始数据传给内核之后，数据越来越少，出现丢失现象，
	// 对原始数据进行备份
	fd_set rdtmp = m_readSet;
	fd_set wrtmp = m_writeSet;

	int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val); //最后是毫秒
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	// 遍历判断那个文件描述符被激活了
	for (int i = 0; i < m_maxSize; ++i)
	{
		if (FD_ISSET(i, &rdtmp)) //读被激活了
		{
			// 读
			m_evLoop->eventActive(i, (int)FDEvent::ReadEvent);

		}

		if (FD_ISSET(i, &wrtmp))
		{
			// 写
			m_evLoop->eventActive(i, (int)FDEvent::WriteEvent);
		}

	}
	return 0;
}

void SelectDispatcher::setFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件 
		// 添加到对应的读写事件内
		FD_SET(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //不用else if,某些时候读写都存在
	{
		FD_SET(m_channel->getSocket(), &m_writeSet);
	}
}

void SelectDispatcher::clearFdset()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent自定义，按位与
	{
		// 清除相应的读事件 
		FD_CLR(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //不用else if,某些时候读写都存在
	{
		FD_CLR(m_channel->getSocket(), &m_writeSet);
	}
}
