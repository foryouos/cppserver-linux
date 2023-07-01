#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
	struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcp->listener = listenerInit(port);
	tcp->mainLoop = eventLoopInit();
	tcp->threadNum = threadNum;
	tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
	return tcp;
}

struct Listener* listenerInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	//1������������fd
	// AF_INET ����IPV4��0 ��ʽЭ���е�TCPЭ��
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket"); //����ʧ��
		return NULL;
	}
	//2�����ö˿ڸ���
	int opt = 1; //1�˿ڸ���
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt"); //����ʧ��
		return NULL;
	}
	//3���󶨶˿�
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //IPV4Э��
	addr.sin_port = htons(port); //���˿�תΪ�����ֽ��� 2��16�η�65536�����
	addr.sin_addr.s_addr = INADDR_ANY; //��������IP��ַ
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind"); //ʧ��
		return NULL;
	}
	//4�����ü���  һ���Կ��ԺͶ��ٿͻ�������,�ں����128�������ܴ��ں˻��Ϊ128
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return NULL;
	}
	//����fd
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}
int acceptConnection(void* arg)
{
	struct TcpServer* server = (struct TcpServer*)arg;
	// �Ϳͻ��˽�������
	int cfd = accept(server->listener->lfd, NULL, NULL);
	// ȡ�����̷߳�Ӧ��ʵ��������cfd
	struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
	// ��cfd�ŵ�TcpConnection���� evLoop�ŵ����߳� �������������̣߳��Ϳͻ���ͨ�Ŷ������߳�������
	tcpConnectionInit(cfd, evLoop);
	return 0;
}
void tcpServerRun(struct TcpServer* server)
{
	Debug("�������Ѿ�����....");	
	// �����̳߳�
	threadPoolRun(server->threadPool);
	
	// ��Ӽ�������
	// ��ʼ��channelʵ��
	struct Channel* channel = channelInit(server->listener->lfd, ReadEvent,
		acceptConnection, NULL,NULL,server);
	eventLoopAddTask(server->mainLoop, channel, ADD); //����ǰ��channel��ӵ������������

	// ������Ӧ��ģ�ͣ���ʼ����
	eventLoopRun(server->mainLoop);  //������Ӧ��ģ��
	

}
