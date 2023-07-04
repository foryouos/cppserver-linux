#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

// �������� �����������ʼ�������ü����������������Լ��������̵߳���������
class TcpServer
{
public:
	TcpServer(unsigned short port, int threadNum);
	// ��ʼ������
	void setListen();
	// ���������� - �����̳߳�-�Լ������׽��ֽ��з�װ�����ŵ����̵߳�������У�������Ӧ��ģ��
	void Run();
	// �������̵߳�����������ú���
	static int acceptConnection(void* arg);

private:
	int m_threadNum;  // �洢���캯����������̸߳���
	EventLoop* m_mainLoop;  // ָ��ָ�����߳�ӵ�е����̷߳�ӳʵ�� 
	ThreadPool* m_threadPool;  // ָ�뱣�����̵߳��̳߳�
	int m_lfd;            //�����������ڼ������ļ�������
	unsigned short m_port; //�˿�
};
