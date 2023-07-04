#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
TcpServer::TcpServer(unsigned short port, int threadNum)
{
	m_port = port;
	m_mainLoop = new EventLoop;
	m_threadNum = threadNum;
	m_threadPool = new ThreadPool(m_mainLoop, threadNum);
	setListen();
}

void TcpServer::setListen()
{
	//1������������fd
	// AF_INET ����IPV4��0 ��ʽЭ���е�TCPЭ��
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_lfd == -1)
	{
		perror("socket"); //����ʧ��
		return ;
	}
	//2�����ö˿ڸ���
	int opt = 1; //1�˿ڸ���
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt"); //����ʧ��
		return ;
	}
	//3���󶨶˿�
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //IPV4Э��
	addr.sin_port = htons(m_port); //���˿�תΪ�����ֽ��� 2��16�η�65536�����
	addr.sin_addr.s_addr = INADDR_ANY; //��������IP��ַ
	ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind"); //ʧ��
		return ;
	}
	//4�����ü���  һ���Կ��ԺͶ��ٿͻ�������,�ں����128�������ܴ��ں˻��Ϊ128
	ret = listen(m_lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return ;
	}
}

void TcpServer::Run()
{
	Debug("�������Ѿ�����....");
	// �����̳߳�
	m_threadPool->Run();
	// ��Ӽ�������
	// ��ʼ��channelʵ��
	Channel* channel = new Channel(m_lfd,FDEvent::ReadEvent,
		acceptConnection, nullptr, nullptr, this); // TODO //ֻҪ�������������м����Ͳ���ֹͣ��
	//����ǰ��channel��ӵ������������
	m_mainLoop->AddTask(channel, ElemType::ADD);
	// ������Ӧ��ģ�ͣ���ʼ����
	m_mainLoop->Run();  //������Ӧ��ģ��
}
int TcpServer::acceptConnection(void* arg)
{
	TcpServer* server = static_cast<TcpServer*>(arg);
	// �Ϳͻ��˽�������
	int cfd = accept(server->m_lfd, NULL, NULL);
	// ȡ�����̷߳�Ӧ��ʵ��������cfd
	EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	// ��cfd�ŵ�TcpConnection���� evLoop�ŵ����߳� �������������̣߳��Ϳͻ���ͨ�Ŷ������߳�������
	// ��������
	new TcpConnection(cfd, evLoop);
	return 0;
}