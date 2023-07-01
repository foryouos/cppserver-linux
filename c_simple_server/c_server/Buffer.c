#define _GNU_SOURCE 
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
struct Buffer* bufferInit(int size)
{
	struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
	if (buffer != NULL)
	{
		//为其申请内存大小
		buffer->data = (char*)malloc(size);
		buffer->capacity = size;
		buffer->writePos = buffer->readPos = 0;
		//数据初始化
		memset(buffer->data, 0, size);
	}
	return buffer;
}

void bufferDestory(struct Buffer* buf)
{
	if (buf != NULL)
	{
		if (buf->data != NULL)
		{
			free(buf->data);
		}
	}
	free(buf);
}

void bufferExtendRoom(struct Buffer* buffer, int size)
{
	//1，可写内存够用 不需要扩容
	if (bufferWriteableSize(buffer) >= size)
	{
		return;
	}
	//2,内存需要合并才够用，不需要扩容
	// 合并剩余的可写内存以及已读内存 > size
	else if (buffer->readPos + bufferWriteableSize(buffer) >= size)
	{
		//得到未读的内存大小
		int readable = bufferReadableSize(buffer);
		// 移动内存,移动位置，移动的数据地址，数据块大小
		memcpy(buffer->data, buffer->data + buffer->readPos,readable);
		// 更新位置
		buffer->readPos = 0;
		buffer->writePos = readable;
	}
	//3，内存不够用，需扩容
	else
	{
		void* temp = realloc(buffer->data, buffer->capacity + size);
		//
		if (temp == NULL)
		{
			return; //失败了
		}
		//对拓展的内存进行初始化temp+buffer->capacity.capacity已经初始化过了
		memset(temp + buffer->capacity,0,size);
		// 更新数据
		buffer->data = temp;
		buffer->capacity += size;
	}
}

int bufferWriteableSize(struct Buffer* buffer)
{
	return buffer->capacity - buffer->writePos;
}

int bufferReadableSize(struct Buffer* buffer)
{
	return buffer->writePos - buffer->readPos;
}

int bufferAppendData(struct Buffer* buffer, const char* data, int size)
{
	//判断buffer和data是不是指向的有效内存
	if (buffer == NULL || data == NULL || size <= 0)
	{
		return -1;
	}
	// 扩容,可以根据需要自动的选择是否需要扩容
	bufferExtendRoom(buffer, size);
	// 数据拷贝 buffer->data + buffer->writePos可写的区域
	memcpy(buffer->data + buffer->writePos, data, size);
	buffer->writePos += size;

	return 0;
}

int bufferAppendString(struct Buffer* buffer, const char* data)
{
	int size = strlen(data);
	int ret = bufferAppendData(buffer, data, size);
	return ret;
}

int bufferSocketRead(struct Buffer* buffer, int fd)
{
	// read/recv/readv - 接受套接字数据
	// read 把数据存入到一个数组
	// readv 会把数据存入到多个数组，效率更高一些
	struct iovec vec[2];
	int writeable = bufferWriteableSize(buffer);
	//初始化数组元素
	vec[0].iov_base = buffer->data + buffer->writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);// 40k数据
	vec[1].iov_base = buffer->data + buffer->writePos;
	vec[1].iov_len = 40960;
	//接受数据
	int result = readv(fd, vec, 2);
	if (result == -1)
	{
		return -1;
	}
	else if(result <= writeable)
	{
		buffer->writePos += result;
	}
	else
	{
		buffer->writePos = buffer->capacity;// buffer被写满，才会进入
		bufferAppendData(buffer, tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}
// 根据"\r\n"取出一行 找到其在数据块中的位置，返回改地址
char* bufferFindCRLF(struct Buffer* buffer)
{
	// strstr --> 大字符串中匹配子字符串(遇到\0结束)
	// 从haystack开始，寻找needle,从左往右开始寻找
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> 大数据块中匹配子数据块(需要指定数据块大小)
	//void *memmem(const void *haystack, size_t haystacklen,
	//const void* needle, size_t needlelen);

	char * ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
	return ptr;
}
// TODO 完善sendfile发送方式，判断文件还是目录
int bufferSendData(struct Buffer* buffer, int socket)
{
	// 判断buffer里面是否有需要发送的数据 得到未读数据即待发送
	int readable = bufferReadableSize(buffer); 
	if (readable > 0)
	{
		//发送出去buffer->data + buffer->readPos 缓存区的位置+已经读到的位置
		// 管道破裂 -- 连接已经断开，服务器继续发数据，出现管道破裂 -- TCP协议
		// 当内核产生信号时，MSG_NOSIGNAL忽略，继续保持连接
		// Linux的信号级别高，Linux大多数信号都会终止信号
		int count = send(socket, buffer->data + buffer->readPos, readable,MSG_NOSIGNAL);
		if (count > 0)
		{
			// 往后移动未读缓存区位置
			buffer->readPos += count;
			// 稍微休眠一下
			usleep(1); // 1微妙
		}
		return count;
	}
	return 0;
}
