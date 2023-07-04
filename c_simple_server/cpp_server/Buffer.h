#pragma once
#include <string>
using namespace std;
// 读写内存结构体，添加字符串，接受套接字数据，将写缓存区数据发送
class Buffer
{
public:
	Buffer(int size);  // 初始化Buffer,size申请大小,可根据Buffer读写指针进行相关操作
	~Buffer();        // 销毁内存函数
	
	// Buffer扩容函数,size实际需要的大小
	void extendRoom(int size);
	// 得到剩余可写的内存容量
	inline int writeableSize()
	{
		return m_capacity - m_readPos;
	};
	// 得到剩余可读的内存容量
	inline int readableSize()
	{
		return m_writePos - m_readPos;
	};
	// 写内存 1，直接写，2，接受套接字数据
	// buffer ,添加的字符串，字符串data对应的长度,把data所指向的数据存入到buffer中去
	int appendString(const char* data, int size);
	// 添加字符串类型数据
	int appendString(const char* data);
	int appendString(const string data);
	//2，接受套接字数据读数据 返回接受数据大小
	int socketRead(int fd);
	// 找到其在数据块中的位置，返回改地址根据 空格换行 取出一行  根据\r\n
	char* findCRLF();
	// 发送数据 缓存区和文件描述符
	int sendData( int socket);
	// 使用Linux的sendfile发送文件
	int sendData(int cfd, int fd, off_t offset, int size);
	//得到读数据的起始位置
	inline char* data()
	{
		return m_data + m_readPos;
	}
	// 移动readPos位置+count
	inline int readPosIncrease(int count)
	{
		m_readPos += count;
		return m_readPos;
	}
private:
	//指向内存的指针
	char* m_data; 
	int m_capacity; //内存容量
	int m_readPos = 0;  //读地址记录从什么位置开始读数据
	int m_writePos = 0; //写地址，记录从什么位置写数据
};

