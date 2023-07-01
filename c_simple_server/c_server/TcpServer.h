#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

struct Listener
{
	int lfd;            //����
	unsigned short port; //�˿�
};
// �������ṹ��
struct TcpServer
{
	int threadNum;  //�̸߳���
	struct EventLoop* mainLoop;   
	struct ThreadPool* threadPool; 
	struct Listener* listener;
};
// ��ʼ��
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
// ��ʼ������
struct Listener*listenerInit(unsigned short port);
// ���������� - �����̳߳�-�Լ������׽��ֽ��з�װ�����ŵ����̵߳�������У�������Ӧ��ģ��
void tcpServerRun(struct TcpServer* server);