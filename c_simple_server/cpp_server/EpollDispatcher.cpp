#include "Dispatcher.h"
#include "EventLoop.h"
#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include "EpollDispatcher.h"
// �û����������evLoop���ݸ����࣬�ø��ౣ������
EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	//������epoll���ڵ�
	m_epfd = epoll_create(10);
	if (m_epfd == -1)
	{
		perror("epoll create");
		exit(0);
	}
	//Ϊ�ṹ��ָ������ڴ棬���飬��������������������ɸ��ṹ���С�����ݿ�
	// calloc��malloc���򵥣����˷����ڴ棬��Ϊ��ʼ�����ڴ��ʼ��Ϊ0
	// Max�ڵ�����������ڵ��ڴ��С
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
	// ͨ��channel�ͷŶ�Ӧ��TcpConnection��Դ
	// argΪ struct TcpConnection* conn = (struct TcpConnection*)arg;
	// const_castȥ�������ֻ������
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
	ev.data.fd = m_channel->getSocket(); //Ҫ�����ļ�������
	//����channel�Ķ�д�¼����Լ����壬��Ҫ�жϽ�������
	// ��λ�� ֻ�ж�Ӧ�Ķ�����11��Ϊ1����������0�������ReadEvent��Ӧ��ֵ
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼�
		events |= EPOLLIN; //��EPOLLIN��Ҳ���������� ��λ����ͬΪ0 ��Ϊ1�� ��¼����ǰ���ͱ�������
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //����else if,ĳЩʱ���д������
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	//�Ѵ�������channel���ļ���������ӵ�epoll���ϣ���Ҫ���channel�¼�
	// ���channel->fd��Ӧ��ʲô�¼� ����ļ���������ʲô�¼�

	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
	
	int count = epoll_wait(m_epfd, m_events,m_maxNode, timeout * 1000); //����Ǻ���
	if (count == -1)
	{
		perror("epoll_wait");
	}
	// �����ļ�������
	for (int i = 0; i < count; ++i)
	{
		//��forѭ���а���Чѭ������ȡ��������Ӧ���¼����ļ���������������
		int events = m_events[i].events;
		int fd = m_events[i].data.fd;
		//�ж��Ƿ�����쳣���������쳣ֱ�Ӵ�epoll����ɾ��
		// EPOLLERR�Զ˶Ͽ�����֮��EPOLLHUP�Զ˶Ͽ�������Ȼ�����ݲ����Ĵ���
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			//�Է��Ͽ������ӣ�ɾ��fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN)
		{
			//��
			m_evLoop->eventActive(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT)
		{
			//д
			m_evLoop->eventActive(fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}

