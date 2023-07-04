#include "Dispatcher.h"
#include "EventLoop.h"
#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include "EpollDispatcher.h"
// 用户传给子类的evLoop传递给父类，让父类保存起来
EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	//创建了epoll根节点
	m_epfd = epoll_create(10);
	if (m_epfd == -1)
	{
		perror("epoll create");
		exit(0);
	}
	//为结构体指针分配内存，数组，根据数组的容量分配若干个结构体大小的数据块
	// calloc比malloc更简单，除了分配内存，还为初始化的内存初始化为0
	// Max节点个数，单个节点内存大小
	m_events = new struct epoll_event[m_maxNode];
	m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
	close(m_epfd);
	delete[]m_events;
}

int EpollDispatcher::add()
{
	int ret = epollCtl(EPOLL_CTL_ADD);
	if (ret == -1)
	{
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
}

int EpollDispatcher::remove()
{
	int ret = epollCtl( EPOLL_CTL_DEL);
	if (ret == -1)
	{
		perror("epoll_ctl delete");
		exit(0);
	}
	// 通过channel释放对应的TcpConnection资源
	// arg为 struct TcpConnection* conn = (struct TcpConnection*)arg;
	// const_cast去掉后面的只读属性
	m_channel->destoryCallback(const_cast<void*>(m_channel->getArg()));
	return ret;
}

int EpollDispatcher::modify()
{
	int ret = epollCtl(EPOLL_CTL_MOD);
	if (ret == -1)
	{
		perror("epoll_ctl Modify");
		exit(0);
	}
	return ret;
}
int EpollDispatcher::epollCtl(int op)
{
	struct epoll_event ev;
	ev.data.fd = m_channel->getSocket(); //要检查的文件描述符
	//由于channel的读写事件由自己定义，需要判断进行设置
	// 按位与 只有对应的二进制11才为1，若不等于0，则等于ReadEvent对应的值
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent自定义，按位与
	{
		//保存相应的读事件
		events |= EPOLLIN; //将EPOLLIN（也是整形数） 按位或（相同为0 异为1） 记录当当前整型变量当中
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //不用else if,某些时候读写都存在
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	//把传过来的channel的文件描述符添加到epoll树上，需要检测channel事件
	// 检测channel->fd对应的什么事件 最后文件描述符的什么事件

	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
	
	int count = epoll_wait(m_epfd, m_events,m_maxNode, timeout * 1000); //最后是毫秒
	if (count == -1)
	{
		perror("epoll_wait");
	}
	// 遍历文件描述符
	for (int i = 0; i < count; ++i)
	{
		//在for循环中把有效循环依次取出，将对应的事件和文件描述符保存下来
		int events = m_events[i].events;
		int fd = m_events[i].data.fd;
		//判断是否出现异常，若出现异常直接从epoll树上删除
		// EPOLLERR对端断开连接之后，EPOLLHUP对端断开连接仍然发数据产生的错误
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			//对方断开了连接，删除fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN)
		{
			//读
			m_evLoop->eventActive(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT)
		{
			//写
			m_evLoop->eventActive(fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}

