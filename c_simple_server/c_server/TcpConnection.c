#include "TcpConnection.h"
#include "HttpRequest.h"
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
// �������ݲ���
int processRead(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// �������� ���Ҫ�洢��readBuf����
	int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
	// data��ʼ��ַ readPos�ö��ĵ�ַλ��
	Debug("���յ���http��������: %s", conn->readBuf->data + conn->readBuf->readPos);

	if (count > 0)
	{
		// ������http���󣬽���http����
		int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
		//��Ӽ��д�¼�
		writeEventEnable(conn->channel, true);
		//  MODIFY�޸ļ���д�¼�
		eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif
		bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, socket);
		if (!flag)
		{
			//����ʧ�ܣ��ظ�һ���򵥵�HTML
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendString(conn->writeBuf, errMsg);
		}
	}
	else
	{
#ifdef MSG_SEND_AUTO  //��������壬
		eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	}
	// �Ͽ����� ��ȫд�뻺�����ٷ��Ͳ��������رգ���û�з���
#ifndef MSG_SEND_AUTO  //���û�б����壬
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	return 0;
}
//д�ص�����������д�¼�����writeBuf�е����ݷ��͸��ͻ���
int processWrite(void* arg)
{
	Debug("��ʼ����������(����д�¼�����)....");
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// ��������
	int count = bufferSendData(conn->writeBuf, conn->channel->fd);
	if (count > 0)
	{
		// �ж������Ƿ�ȫ�������ͳ�ȥ
		if (bufferReadableSize(conn->writeBuf) == 0)
		{
			// ���ݷ������
			// 1�����ټ��д�¼� --�޸�channel�б�����¼�
			writeEventEnable(conn->channel, false);
			// 2, �޸�dispatcher�м��ļ��ϣ���enentLoop��ӳģ����Ϊ���нڵ���Ϊmodify
			eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
			//3������ͨ�ţ�ɾ������ڵ�
			eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
		}
	}
	return 0;
}


struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evloop)
{
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	conn->evLoop = evloop;
	conn->readBuf = bufferInit(10240); //10K
	conn->writeBuf = bufferInit(10240);
	// http ��ʼ��
	conn->request = httpRequestInit();
	conn->response = httpResponseInit();

	// ���ӵ�����
	sprintf(conn->name, "Connection-%d", fd);
	// ��������������֪����ʱ�򣬿ͻ�����û�����ݵ���
	conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestory,conn);
	// ��channel�ŵ�����ѭ���������������
	eventLoopAddTask(evloop, conn->channel, ADD);
	Debug("�Ϳͻ��˽�������,threadName: %s,threadID:%s,connName:%s", evloop->threadName,evloop->threadID,conn->name);
	return conn;
}
// tcp�Ͽ�����ʱ�Ͽ�
int tcpConnectionDestory(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// �ж�conn�Ƿ�Ϊ��
	if (conn != NULL)
	{
		// �ж� ��д�������Ƿ�������û�б�����
		if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0
			&& conn->writeBuf && bufferReadableSize(conn->writeBuf) == 0)
		{
			destoryChannel(conn->evLoop, conn->channel);
			bufferDestory(conn->readBuf);
			bufferDestory(conn->writeBuf);
			httpRequetDestory(conn->request);
			httpResponseDestory(conn->response);
			free(conn);
		}
	}
	Debug("���ӶϿ����ͷ���Դ, connName�� %s", conn->name);
	return 0;
}
