#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>

HttpResponse::HttpResponse()
{
	m_statusCode = StatusCode::Unknown;//初始化时为0
	// 初始化数组
	m_headers.clear();
	//状态描述 对文件名初始化
	m_fineName = string();
	
	// 对回调函数初始化
	// 函数指针
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
	//准备HTTP的响应数据
	//状态行
	char tmp[1024] = { 0 };
	int code = static_cast<int>(m_statusCode);
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
	sendBuf->appendString(tmp);
	//响应头
	for (auto it = m_headers.begin(); it!= m_headers.end(); ++it)
	{
		sprintf(tmp,"%s: %s\r\n",it->first.data(),it->second.data());
		sendBuf->appendString(tmp);
	}

	// 空行
	sendBuf->appendString("\r\n");
	// 发送出去
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(socket);
#endif // !MSG_SEND_AUTO
	// 回复的数据
	sendDataFunc(m_fineName, sendBuf, socket);
}
