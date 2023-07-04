#include "Channel.h"
#include <stdlib.h>

Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void* arg)
{
	//�Խṹ���ʼ��
	m_arg = arg;  //�����˷�ӳ��ģ�ͳ�ʼ������
	m_fd = fd;
	m_events = (int)events;
	readCallback = readFunc;
	writeCallback = writeFunc;
	destoryCallback = destoryFunc;
}
//���д����
// ����ӦΪ10 ��Ҫд���д���ԣ���100��򣬵�110��д����
// �粻д������λ���㣬��Ϊ110������λ���㣬��дȡ��011���ڰ�λ��& 010ֻ���¶��¼�
void Channel::writeEventEnable(bool flag)
{
	if (flag) //���Ϊ�棬���д����
	{
		// ��� ��ͬΪ0 ��Ϊ1
		// WriteEvent ������������������־λ1��ͨ����� ��channel->events�ĵ���λΪ1
		m_events |= static_cast<int>(FDEvent::WriteEvent); // ��λ��� int events����32λ��0/1,
	}
	else // �����д����channel->events ��Ӧ�ĵ���λ����
	{
		// ~WriteEvent ��λ�룬 ~WriteEventȡ�� 011 Ȼ���� channel->events��λ��&���� ֻ��11 Ϊ 1��������Ϊ0ֻ��ͬΪ��ʱ���棬һ����٣�1Ϊ�棬0Ϊ��
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent);  //channel->events ����λ����֮��д�¼��Ͳ��ټ��
	}
}
bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent);  //��λ�� ������λ����1������д�����������������0����������������Ϊ0
}