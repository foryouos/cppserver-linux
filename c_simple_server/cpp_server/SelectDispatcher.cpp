#include "Dispatcher.h"
#include <sys/select.h>
#include "EventLoop.h"
#include <stdio.h>
#include <stdlib.h>
#include "SelectDispatcher.h"
SelectDispatcher::SelectDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	//�Լ��Ͻ�����գ����б�־λ����Ϊ0
	FD_ZERO(&m_readSet);
	FD_ZERO(&m_writeSet);  //�ļ��������ĵ�ַ
	m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
	if (m_channel->getSocket() >= m_maxSize) //select������ļ���������1024
	{
		return -1;
	}
	setFdSet();
	return 0;
}

int SelectDispatcher::remove()
{
	clearFdset();
	// ͨ��channel�ͷŶ�Ӧ��TcpConnection��Դ
	// argΪ struct TcpConnection* conn = (struct TcpConnection*)arg;
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
	//��ʱʱ��
	struct timeval val;
	val.tv_sec = timeout; //��
	val.tv_usec = 0; //΢��,�ò���Ҳ�ó�ʼ��Ϊ0��ϵͳ����ӣ�����������

	// 234���봫���������ں˻��޸Ĵ������ݣ�ԭʼ���ݴ����ں�֮������Խ��Խ�٣����ֶ�ʧ����
	// ��ԭʼ���ݽ��б���
	fd_set rdtmp = m_readSet;
	fd_set wrtmp = m_writeSet;

	int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val); //����Ǻ���
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	// �����ж��Ǹ��ļ���������������
	for (int i = 0; i < m_maxSize; ++i)
	{
		if (FD_ISSET(i, &rdtmp)) //����������
		{
			// ��
			m_evLoop->eventActive(i, (int)FDEvent::ReadEvent);

		}

		if (FD_ISSET(i, &wrtmp))
		{
			// д
			m_evLoop->eventActive(i, (int)FDEvent::WriteEvent);
		}

	}
	return 0;
}

void SelectDispatcher::setFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		//������Ӧ�Ķ��¼� 
		// ��ӵ���Ӧ�Ķ�д�¼���
		FD_SET(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //����else if,ĳЩʱ���д������
	{
		FD_SET(m_channel->getSocket(), &m_writeSet);
	}
}

void SelectDispatcher::clearFdset()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) //ReadEvent�Զ��壬��λ��
	{
		// �����Ӧ�Ķ��¼� 
		FD_CLR(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) //����else if,ĳЩʱ���д������
	{
		FD_CLR(m_channel->getSocket(), &m_writeSet);
	}
}
