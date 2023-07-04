#pragma once
// 读写内存结构体
struct Buffer
{
	//指向内存的指针
	char* data;
	int capacity; //内存容量
	int readPos;  //读地址记录从什么位置开始读数据
	int writePos; //写地址，记录从什么位置写数据
};

// 初始化Buffer,size申请大小,可根据Buffer读写指针进行相关操作
struct Buffer* bufferInit(int size);
// 销毁内存函数
void bufferDestory(struct Buffer* buf);
// Buffer扩容函数,size实际需要的大小
void bufferExtendRoom(struct Buffer* buffer, int size);
// 得到剩余可写的内存容量
int bufferWriteableSize(struct Buffer* buffer);
// 得到剩余可读的内存容量
int bufferReadableSize(struct Buffer* buffer);
// 写内存 1，直接写，2，接受套接字数据
// buffer ,添加的字符串，字符串data对应的长度,把data所指向的数据存入到buffer中去
int bufferAppendData(struct Buffer* buffer,const char* data,int size);
// 添加字符串类型数据
int bufferAppendString(struct Buffer* buffer, const char* data);
//2，接受套接字数据读数据 返回接受数据大小
int bufferSocketRead(struct Buffer* buffer, int fd);
// 找到其在数据块中的位置，返回改地址根据空格换行取出一行 
char* bufferFindCRLF(struct Buffer* buffer);
// 发送数据 缓存区和文件描述符
int bufferSendData(struct Buffer* buffer, int socket);
// 使用Linux的sendfile发送文件
int bufferSendFileData(int socket,int fd,int offset,int size);