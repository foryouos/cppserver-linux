#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
//�����ֵ��
struct RequestHeader
{
	char* key;
	char* value;
};
// ��ǰ�Ľ���״̬
enum HttpRequestState
{
	ParseReqLine,     //��ǰ��������������
	ParseReqHeaders,  // ��ǰ������������ͷ
	ParseReqBody,     // ��ǰ����������������ݿ�
	ParseReqDone,     // ��ǰHttp�����Ѿ���������

};

// ����http ����ṹ��
struct HttpRequest
{
	char* method; //���󷽷�
	char* url;    //��������
	char* version;//����汾
	struct RequestHeader* reqheaders; //���
	int reqHeadersNum;    //�洢���ٸ���Ч�������ֵ��
	enum HttpRequestState curState;
};

// ��ʼ������
struct HttpRequest* httpRequestInit();
// ����http����ṹ��
void httpRequetReset(struct HttpRequest* req);
// �ͷ��ڴ�����
void httpRequetResetEx(struct HttpRequest* req);
// �ڴ��ͷ�
void httpRequetDestory(struct HttpRequest* req);
// ��ȡ����״̬
enum HttpRequestState httpRequestState(struct HttpRequest* request);
//�������ͷ
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
// ����key�õ�����ͷ��value
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);
// ����������,bool��ʾʧ����ɹ�����readBud�������ж�ȡ���ݣ������֣�����ʽ��url����httpЭ��
bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer* readBuf);
// ��������ͷ������һ�У����У���ε��ü���)
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
// ����http����Э��
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket);
// ����Http����Э�� Get��http����
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);

// ����
// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from);
//�����ļ���׺���õ��ļ�����
const char* getFileType(const char* name);
// ���ַ�ת��Ϊ������
int hexToDec(char c);
//Ҫ�ظ�http��Ӧ�����ݿ�
void sendFile(const char* filename, struct Buffer* sendBuf, int cfd);
// ����Ӻõ����ݿ�ŵ����ͻ�������
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);