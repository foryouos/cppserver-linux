#pragma once  //���ֹ���ظ�����
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef int(*handleFunc)(void* arg); //�������ͣ�handleFunc������

//�����ļ��������Ķ�д�¼���ʹ��ö��   �Զ���
enum FDEvent
{
	TimeOut = 0x01,       //ʮ����1����ʱ�� 1
	ReadEvent = 0x02,    //ʮ����4        10
	WriteEvent = 0x04   //ʮ����4  ������ 100
};

//���������ļ��������Ͷ�д���Ͷ�д�ص������Լ�����
struct Channel
{
	//�ļ�������
	int fd;
	// �¼�
	int events;   //��/д/��д
	//�ص�����
	handleFunc readCallback;   //���ص�
	handleFunc writeCallback;  //д�ص�
	handleFunc destoryCallback;  //���ٻص�
	// �ص���������
	void* arg;
}Channel;
//��ʼ��һ��Channel
struct Channel* channelInit(int fd,int events,handleFunc readFunc,handleFunc writeFunc, handleFunc destoryFunc,void* arg);
//�޸�fd��д�¼�(��� or �����)��flag�Ƿ�Ϊд�¼�
void writeEventEnable(struct Channel* channel, bool flag);

//�ж�ʱ����Ҫ����ļ���������д�¼�
bool isWriteEventEnable(struct Channel* channel);