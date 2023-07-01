#include "Channel.h"
#include <stdlib.h>
struct Channel* channelInit(int fd, int events, handleFunc readFunc,
	handleFunc writeFunc, handleFunc destoryFunc, void* arg)
{
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel)); //���붯̬�ռ��ڴ�
	//�Խṹ���ʼ��
	channel->arg = arg;  //�����˷�ӳ��ģ�ͳ�ʼ������
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readFunc;
	channel->writeCallback = writeFunc;
	channel->destoryCallback = destoryFunc;
	//���س�ʼ���Ľṹ��
	return channel;
}

void writeEventEnable(struct Channel* channel, bool flag)
{
	if (flag) //���Ϊ�棬���д����
	{
		// ��� ��ͬΪ0 ��Ϊ1
		// WriteEvent ������������������־λ1��ͨ����� ��channel->events�ĵ���λΪ1
		channel->events |= WriteEvent; // ��λ��� int events����32λ��0/1,
	}
	else // �����д����channel->events ��Ӧ�ĵ���λ����
	{ 
		// ~WriteEvent ��λ�룬 ~WriteEventȡ�� 011 Ȼ���� channel->events��λ��&���� ֻ��11 Ϊ 1��������Ϊ0ֻ��ͬΪ��ʱ���棬һ����٣�1Ϊ�棬0Ϊ��
		channel->events = channel->events & ~WriteEvent;  //channel->events ����λ����֮��д�¼��Ͳ��ټ��
	} 

}

bool isWriteEventEnable(struct Channel* channel)
{

	return channel->events & WriteEvent;  //��λ�� ������λ����1������д�����������������0����������������Ϊ0
}
