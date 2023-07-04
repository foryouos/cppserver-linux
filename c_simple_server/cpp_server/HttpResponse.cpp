#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>

HttpResponse::HttpResponse()
{
	m_statusCode = StatusCode::Unknown;//��ʼ��ʱΪ0
	// ��ʼ������
	m_headers.clear();
	//״̬���� ���ļ�����ʼ��
	m_fineName = string();
	
	// �Իص�������ʼ��
	// ����ָ��
	sendDataFunc = nullptr;

}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_headers.insert(make_pair(key, value));

}

void HttpResponse::prepareMsg(Buffer* sendBuf, int socket)
{
	//׼��HTTP����Ӧ����
	//״̬��
	char tmp[1024] = { 0 };
	int code = static_cast<int>(m_statusCode);
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
	sendBuf->appendString(tmp);
	//��Ӧͷ
	for (auto it = m_headers.begin(); it!= m_headers.end(); ++it)
	{
		sprintf(tmp,"%s: %s\r\n",it->first.data(),it->second.data());
		sendBuf->appendString(tmp);
	}

	// ����
	sendBuf->appendString("\r\n");
	// ���ͳ�ȥ
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(socket);
#endif // !MSG_SEND_AUTO
	// �ظ�������
	sendDataFunc(m_fineName, sendBuf, socket);
}
