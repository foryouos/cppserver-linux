#pragma once
#include "Buffer.h"
#include <string>
#include <map>
#include <functional>
using namespace std;
// 定义状态码枚举
enum class StatusCode
{
	Unknown,
	OK = 200,
	MovedPermannently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};
// 定义一个函数指针，用来组织要回复给客户端的数据块
// fileName要给客户端回复的静态资源内容
// sendBuf 内存，存储发送数据的Buffer缓冲
// socket 文件描述符，用于通信，将sendBuf数据发送给客户端

//定义http响应，添加响应头，准备响应数据
class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();
	function<void(const string fileName,  Buffer* , int)> sendDataFunc;

	// 添加响应头
	void addHeader( const string key, const string value);
	// 组织http响应数据
	// HttpResponse  组织数据块，写到内存中，写到sendBuf
	// sendBuf 发送数据的 socket用于通信
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
	// 状态行：状态码，状态描述，http协议
	StatusCode m_statusCode;
	string m_fineName;  // 文件名
	// 响应头 - 键值对(存储响应头内部所有数据
	map<string,string> m_headers;
	// 定义状态码和描述对应关系
	const map<int, string> m_info =
	{
		{200,"OK"},
		{301,"MovedPermannently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};
};

