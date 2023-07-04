#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
#include <map>
using namespace std;
// ��ǰ�Ľ���״̬
enum class PressState:char
{
	ParseReqLine,     //��ǰ��������������
	ParseReqHeaders,  // ��ǰ������������ͷ
	ParseReqBody,     // ��ǰ����������������ݿ�
	ParseReqDone,     // ��ǰHttp�����Ѿ���������

};

// ����http ����ṹ���������ͷ��㣬���������У�ͷ������/����http����Э�飬��ȡ�ļ�����
// �����ļ�/Ŀ¼ ��������url,Method��Version ,state
class HttpRequest
{
public:
	HttpRequest();
	~HttpRequest();
	// ����http����ṹ��	// �ͷ��ڴ�����
	void Reset();
	//�������ͷ
	void addHeader( const string key, const string value);
	// ����key�õ�����ͷ��value
	string getHeader( const string key);
	// ����������,bool��ʾʧ����ɹ�����readBud�������ж�ȡ���ݣ������֣�����ʽ��url����httpЭ��
	bool parseRequestLine(Buffer* readBuf);
	// ��������ͷ������һ�У����У���ε��ü���)
	bool parseRequestHeader(Buffer* readBuf);
	// ����http����Э��
	bool parseHttpRequest(Buffer* readBuf,
		HttpResponse* response, Buffer* sendBuf, int socket);
	// ����Http����Э�� Get��http����
	bool processHttpRequest(HttpResponse* response);

	// ����
	// to �洢����֮�������, ��������, from�����������, �������
	string decodeMsg(string from);
	//�����ļ���׺���õ��ļ�����
	const string getFileType(const string name);

	//Ҫ�ظ�http��Ӧ�����ݿ�
	static void sendFile(string filename, Buffer* sendBuf, int cfd);
	// ����Ӻõ����ݿ�ŵ����ͻ�������
	static void sendDir(string dirName, Buffer* sendBuf, int cfd);

	inline void setMethod(string method)
	{
		m_method = method;
	}
	inline void seturl(string url)
	{
		m_url = url;
	}
	inline void setVersion(string version)
	{
		m_version= version;
	}
	// ��ȡ����״̬
	inline PressState getState()
	{
		return m_curState;
	};
	//�޸�״̬
	inline void setState(PressState state)
	{
		m_curState = state;
	}
private:
	//���������
	// ����һ��ָ�룬ָ��(4���ֽ�)�ڴ��ݹ����л����һ����������ָ�����һ��������������ָ��ĵ�ַ
	// �ں����ڲ����ⲿ��һ��ָ������ڴ棬���ⲿһ��ָ��ĵ�ַ��������������ָ�룬
	// ���� ������ָ��
	char* splitRequestLine(const char* start, const char* end,
		const char* sub,function<void(string)> callback);
	// ���ַ�ת��Ϊ������
	int hexToDec(char c);

private:
	string m_method; //���󷽷�
	string m_url;    //��������
	string m_version;//����汾
	map<string,string> m_reqHeaders; //���
	PressState m_curState;
};

