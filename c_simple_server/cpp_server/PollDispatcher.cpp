#include "Dispatcher.h"
#include <poll.h>
#include "EventLoop.h"
#include <stdlib.h>
#include <stdio.h>
#include "PollDispatcher.h"

PollDispatcher::PollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	m_maxfd = 0;
	m_fds = new struct pollfd[m_maxNode];
	//对应数组初始化
	for (int i = 0; i < m_maxNode; ++i)
	{
		m_fds[i].fd = -1; //初始化-1为无效的文件描述符
		m_fds[i].events = 0; //当前文件描述符检测的事件
		m_fds[i].revents = 0;  //返回的事件

	}
	m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
	delete[]m_fds;
}

int PollDispatcher::add()
{
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件
		events |= POLLIN; //将EPOLLIN（也是整形数） 按位或（相同为0 异为1） 记录当当前整型变量当中
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //不用else if,某些时候读写都存在
	{
		events |= POLLOUT;
	}
	//找出空闲的可用元素
	int i;
	for (i = 0; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == -1)
		{
			//赋值操作
			m_fds[i].events = events;
			m_fds[i].fd = m_channel->getSocket();
			//更新data指向数组里maxfd，比较和最后一个数组里的下标 
			m_maxfd = i > m_maxfd ? i : m_maxfd;
			break;
		}
	}
	if (i >= m_maxNode)
	{
		return -1;
	}

	return 0;
}

int PollDispatcher::remove()
{
	int i;
	for (i = 0; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == m_channel->getSocket())
		{
			//赋值操作
			m_fds[i].events = 0;
			m_fds[i].revents = 0;
			m_fds[i].fd = -1;
			break;
		}
	}
	// 通过channel释放对应的TcpConnection资源
	// arg为 struct TcpConnection* conn = (struct TcpConnection*)arg;
	m_channel->destoryCallback(const_cast<void*>(m_channel->getArg()));
	if (i >= m_maxNode)
	{
		return -1;
	}
	return 0;
}

int PollDispatcher::modify()
{
	
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件
		events |= POLLIN; //将EPOLLIN（也是整形数） 按位或（相同为0 异为1） 记录当当前整型变量当中
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //不用else if,某些时候读写都存在
	{
		events |= POLLOUT;
	}
	//找出空闲的可用元素
	int i;
	for (i = 0; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == m_channel->getSocket())
		{
			//赋值操作
			m_fds[i].events = events;
			break;
		}
	}
	if (i >= m_maxNode)
	{
		return -1;
	}

	return 0;
}

int PollDispatcher::dispatch(int timeout)
{

	//第二个参数 内存需要根据此参数确定遍历数组的时候的范围
	int count = poll(m_fds, m_maxfd + 1, timeout * 1000); //最后是毫秒
	if (count == -1)
	{
		perror("poll");
		exit(0);
	}
	for (int i = 0; i <= m_maxfd; ++i)
	{
		if (m_fds->fd == -1)
		{
			continue;
		}

		if (m_fds[i].revents & POLLIN)
		{
			// 读
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::ReadEvent);
		}
		if (m_fds[i].revents & POLLOUT)
		{
			// 写
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
