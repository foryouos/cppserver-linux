#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define ResHeaderSize 16
struct HttpResponse* httpResponseInit()
{
	struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
	response->headerNum = 0;
	int size = sizeof(struct ResponseHeader) * ResHeaderSize;
	response->headers = (struct ResponseHeader*)malloc(size);
	response->statusCode = Unknown;//初始化时为0
	// 初始化数组
	bzero(response->headers, size);
	//状态描述
	bzero(response->statusMsg, sizeof(response->statusMsg));
	// 对文件名初始化
	bzero(response->fineName, sizeof(response->fineName));
	// 对回调函数初始化
	// 函数指针
	response->sendDataFunc = NULL;

	return response;
}

void httpResponseDestory(struct HttpResponse* response)
{
	if (response != NULL) //如果不为空，数据有效
	{
		// 释放结构体内部
		free(response->headers);
		// 释放结构体
		free(response);
	}
}

void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value)
{
	if (response == NULL || key == NULL || value == NULL)
	{
		return;
	}
	strcpy(response->headers[response->headerNum].key, key);
	strcpy(response->headers[response->headerNum].value, value);
	response->headerNum++;
}

void httpResponsePrepareMsg(struct HttpResponse* response,struct  Buffer* sendBuf, int socket)
{
	//状态行
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
	bufferAppendString(sendBuf, tmp);
	//响应头
	for (int i = 0; i < response->headerNum; ++i)
	{
		sprintf(tmp, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
		bufferAppendString(sendBuf, tmp);
	}

	// 空行
	bufferAppendString(sendBuf, "\r\n");
	// 发送出去
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, socket);
#endif // !MSG_SEND_AUTO
	

	// 回复的数据
	response->sendDataFunc(response->fineName, sendBuf, socket);

}
