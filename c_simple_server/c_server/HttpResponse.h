#pragma once
#include "Buffer.h"
// ����״̬��ö��
enum HttpStatusCode
{
	Unknown,
	OK = 200,
	MovedPermannently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};
// ������Ӧ�Ľṹ�� ��ֵ��
struct ResponseHeader
{
	char key[32];
	char value[128];
};
// ����һ������ָ�룬������֯Ҫ�ظ����ͻ��˵����ݿ�
// fileNameҪ���ͻ��˻ظ��ľ�̬��Դ����
// sendBuf �ڴ棬�洢�������ݵ�Buffer����
// socket �ļ�������������ͨ�ţ���sendBuf���ݷ��͸��ͻ���
typedef void(*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);


//����ṹ��
struct HttpResponse
{
	// ״̬�У�״̬�룬״̬������httpЭ��
	enum HttpStatusCode statusCode;
	char statusMsg[128];  //״̬��������
	char fineName[128];  // �ļ���
	// ��Ӧͷ - ��ֵ��(�洢��Ӧͷ�ڲ���������

	struct ResponseHeader* headers;
	int headerNum; //��Ӧͷ�ĸ���

	responseBody sendDataFunc;
};

//��ʼ��HttpResponse�ṹ��ĳ�ʼ������
struct HttpResponse* httpResponseInit();
// ���ٺ���
void httpResponseDestory(struct HttpResponse* response);

// �����Ӧͷ
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value);
// ��֯http��Ӧ����
// HttpResponse  ��֯���ݿ飬д���ڴ��У�д��sendBuf
// sendBuf �������ݵ� socket����ͨ��
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);