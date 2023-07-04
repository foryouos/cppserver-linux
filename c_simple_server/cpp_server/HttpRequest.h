#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
#include <map>
using namespace std;
// 当前的解析状态
enum class PressState:char
{
	ParseReqLine,     //当前解析的是请求行
	ParseReqHeaders,  // 当前解析的是请求头
	ParseReqBody,     // 当前解析的是请求的数据块
	ParseReqDone,     // 当前Http请求已经解析完了

};

// 定义http 请求结构体添加请求头结点，解析请求行，头，解析/处理http请求协议，获取文件类型
// 发送文件/目录 设置请求url,Method，Version ,state
class HttpRequest
{
public:
	HttpRequest();
	~HttpRequest();
	// 重置http请求结构体	// 释放内存重置
	void Reset();
	//添加请求头
	void addHeader( const string key, const string value);
	// 根据key得到请求头的value
	string getHeader( const string key);
	// 解析请求行,bool表示失败与成果，从readBud读缓存中读取数据，三部分，请求方式，url，和http协议
	bool parseRequestLine(Buffer* readBuf);
	// 解析请求头，请求一行（多行，多次调用即可)
	bool parseRequestHeader(Buffer* readBuf);
	// 解析http请求协议
	bool parseHttpRequest(Buffer* readBuf,
		HttpResponse* response, Buffer* sendBuf, int socket);
	// 处理Http请求协议 Get的http请求
	bool processHttpRequest(HttpResponse* response);

	// 解码
	// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
	string decodeMsg(string from);
	//根据文件后缀，得到文件类型
	const string getFileType(const string name);

	//要回复http响应的数据块
	static void sendFile(string filename, Buffer* sendBuf, int cfd);
	// 把添加好的数据块放到发送缓存区中
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
	// 获取处理状态
	inline PressState getState()
	{
		return m_curState;
	};
	//修改状态
	inline void setState(PressState state)
	{
		m_curState = state;
	}
private:
	//拆分请求行
	// 传入一级指针，指针(4个字节)在传递过程中会产生一个副本，是指针产生一个副本，而不是指针的地址
	// 在函数内部给外部的一级指针分配内存，把外部一级指针的地址传给参数即二级指针，
	// 返回 结束的指针
	char* splitRequestLine(const char* start, const char* end,
		const char* sub,function<void(string)> callback);
	// 将字符转换为整形数
	int hexToDec(char c);

private:
	string m_method; //请求方法
	string m_url;    //请求连接
	string m_version;//请求版本
	map<string,string> m_reqHeaders; //多个
	PressState m_curState;
};

