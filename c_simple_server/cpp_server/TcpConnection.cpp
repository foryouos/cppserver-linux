#include "TcpConnection.h"
#include "log.h"

TcpConnection::TcpConnection(int fd, EventLoop* evloop)
{
	//��û�д���evloop����ǰ��TcpConnect���������߳�����ɵ�
	m_evLoop = evloop;
	m_readBuf = new Buffer(10240); //10K
	m_writeBuf = new Buffer(10240);
	// ��ʼ��
	m_request = new HttpRequest;
	m_response = new HttpResponse;

	m_name = "Connection-" + to_string(fd);

	// ��������������֪����ʱ�򣬿ͻ�����û�����ݵ���
	m_channel =new Channel(fd,FDEvent::ReadEvent, processRead, processWrite, destory, this);
	// ��channel�ŵ�����ѭ���������������
	evloop->AddTask(m_channel, ElemType::ADD);
}
// �������ݲ���
TcpConnection::~TcpConnection()
{
	// �ж� ��д�������Ƿ�������û�б�����
	if (m_readBuf && m_readBuf->readableSize() == 0
		&& m_writeBuf && m_writeBuf->readableSize() == 0)
	{
		delete m_writeBuf;
		delete m_readBuf;
		delete m_request;
		delete m_response;
		m_evLoop->freeChannel(m_channel);
	}

	Debug("���ӶϿ����ͷ���Դ, connName�� %s", m_name);
}

int TcpConnection::processRead(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// �������� ���Ҫ�洢��readBuf����
	int socket = conn->m_channel->getSocket();
	int count = conn->m_readBuf->socketRead(socket);
	// data��ʼ��ַ readPos�ö��ĵ�ַλ��
	Debug("���յ���http��������: %s", conn->m_readBuf->data());

	if (count > 0)
	{
		// ������http���󣬽���http����
		
#ifdef MSG_SEND_AUTO
		//��Ӽ��д�¼�
		conn->m_channel->writeEventEnable(true);
		//  MODIFY�޸ļ���д�¼�
		conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
#endif
		bool flag = conn->m_request->parseHttpRequest(
			conn->m_readBuf, conn->m_response,
			conn->m_writeBuf, socket);
		if (!flag)
		{
			//����ʧ�ܣ��ظ�һ���򵥵�HTML
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else
	{
		
#ifdef MSG_SEND_AUTO  //��������壬
		//�Ͽ�����
		conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	}
	// �Ͽ����� ��ȫд�뻺�����ٷ��Ͳ��������رգ���û�з���
#ifndef MSG_SEND_AUTO  //���û�б����壬
	conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}

//д�ص�����������д�¼�����writeBuf�е����ݷ��͸��ͻ���
int TcpConnection::processWrite(void* arg)
{
	Debug("��ʼ����������(����д�¼�����)....");
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// ��������
	int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0)
	{
		// �ж������Ƿ�ȫ�������ͳ�ȥ
		if (conn->m_writeBuf->readableSize() == 0)
		{
			// ���ݷ������
			// 1�����ټ��д�¼� --�޸�channel�б�����¼�
			conn->m_channel->writeEventEnable(false);
			// 2, �޸�dispatcher�м��ļ��ϣ���enentLoop��ӳģ����Ϊ���нڵ���Ϊmodify
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
			//3������ͨ�ţ�ɾ������ڵ�
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}

// tcp�Ͽ�����ʱ�Ͽ�
int TcpConnection::destory(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	if (conn != nullptr)
	{
		delete conn;
	}
	return 0;
}
