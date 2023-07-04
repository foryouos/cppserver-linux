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
	//��Ӧ�����ʼ��
	for (int i = 0; i < m_maxNode; ++i)
	{
		m_fds[i].fd = -1; //��ʼ��-1Ϊ��Ч���ļ�������
		m_fds[i].events = 0; //��ǰ�ļ������������¼�
		m_fds[i].revents = 0;  //���ص��¼�

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
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼�
		events |= POLLIN; //��EPOLLIN��Ҳ���������� ��λ����ͬΪ0 ��Ϊ1�� ��¼����ǰ���ͱ�������
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //����else if,ĳЩʱ���д������
	{
		events |= POLLOUT;
	}
	//�ҳ����еĿ���Ԫ��
	int i;
	for (i = 0; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == -1)
		{
			//��ֵ����
			m_fds[i].events = events;
			m_fds[i].fd = m_channel->getSocket();
			//����dataָ��������maxfd���ȽϺ����һ����������±� 
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
			//��ֵ����
			m_fds[i].events = 0;
			m_fds[i].revents = 0;
			m_fds[i].fd = -1;
			break;
		}
	}
	// ͨ��channel�ͷŶ�Ӧ��TcpConnection��Դ
	// argΪ struct TcpConnection* conn = (struct TcpConnection*)arg;
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
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼�
		events |= POLLIN; //��EPOLLIN��Ҳ���������� ��λ����ͬΪ0 ��Ϊ1�� ��¼����ǰ���ͱ�������
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //����else if,ĳЩʱ���д������
	{
		events |= POLLOUT;
	}
	//�ҳ����еĿ���Ԫ��
	int i;
	for (i = 0; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == m_channel->getSocket())
		{
			//��ֵ����
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

	//�ڶ������� �ڴ���Ҫ���ݴ˲���ȷ�����������ʱ��ķ�Χ
	int count = poll(m_fds, m_maxfd + 1, timeout * 1000); //����Ǻ���
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
			// ��
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::ReadEvent);
		}
		if (m_fds[i].revents & POLLOUT)
		{
			// д
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
