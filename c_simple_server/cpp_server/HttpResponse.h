#pragma once
#include "Buffer.h"
#include <string>
#include <map>
#include <functional>
using namespace std;
// ����״̬��ö��
enum class StatusCode
{
	Unknown,
	OK = 200,
	MovedPermannently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};
// ����һ������ָ�룬������֯Ҫ�ظ����ͻ��˵����ݿ�
// fileNameҪ���ͻ��˻ظ��ľ�̬��Դ����
// sendBuf �ڴ棬�洢�������ݵ�Buffer����
// socket �ļ�������������ͨ�ţ���sendBuf���ݷ��͸��ͻ���

//����http��Ӧ�������Ӧͷ��׼����Ӧ����
class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();
	function<void(const string fileName,  Buffer* , int)> sendDataFunc;

	// �����Ӧͷ
	void addHeader( const string key, const string value);
	// ��֯http��Ӧ����
	// HttpResponse  ��֯���ݿ飬д���ڴ��У�д��sendBuf
	// sendBuf �������ݵ� socket����ͨ��
	void prepareMsg(Buffer* sendBuf, int socket);
	inline void setFileName(string name)
	{
		m_fineName = name;
	}
	inline void setStatusCode(StatusCode code)
	{
		m_statusCode = code;
	}

private:
	// ״̬�У�״̬�룬״̬������httpЭ��
	StatusCode m_statusCode;
	string m_fineName;  // �ļ���
	// ��Ӧͷ - ��ֵ��(�洢��Ӧͷ�ڲ���������
	map<string,string> m_headers;
	// ����״̬���������Ӧ��ϵ
	const map<int, string> m_info =
	{
		{200,"OK"},
		{301,"MovedPermannently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};
};

