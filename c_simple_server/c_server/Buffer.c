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
		//Ϊ�������ڴ��С
		buffer->data = (char*)malloc(size);
		buffer->capacity = size;
		buffer->writePos = buffer->readPos = 0;
		//���ݳ�ʼ��
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
	//1����д�ڴ湻�� ����Ҫ����
	if (bufferWriteableSize(buffer) >= size)
	{
		return;
	}
	//2,�ڴ���Ҫ�ϲ��Ź��ã�����Ҫ����
	// �ϲ�ʣ��Ŀ�д�ڴ��Լ��Ѷ��ڴ� > size
	else if (buffer->readPos + bufferWriteableSize(buffer) >= size)
	{
		//�õ�δ�����ڴ��С
		int readable = bufferReadableSize(buffer);
		// �ƶ��ڴ�,�ƶ�λ�ã��ƶ������ݵ�ַ�����ݿ��С
		memcpy(buffer->data, buffer->data + buffer->readPos,readable);
		// ����λ��
		buffer->readPos = 0;
		buffer->writePos = readable;
	}
	//3���ڴ治���ã�������
	else
	{
		void* temp = realloc(buffer->data, buffer->capacity + size);
		//
		if (temp == NULL)
		{
			return; //ʧ����
		}
		//����չ���ڴ���г�ʼ��temp+buffer->capacity.capacity�Ѿ���ʼ������
		memset(temp + buffer->capacity,0,size);
		// ��������
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
	//�ж�buffer��data�ǲ���ָ�����Ч�ڴ�
	if (buffer == NULL || data == NULL || size <= 0)
	{
		return -1;
	}
	// ����,���Ը�����Ҫ�Զ���ѡ���Ƿ���Ҫ����
	bufferExtendRoom(buffer, size);
	// ���ݿ��� buffer->data + buffer->writePos��д������
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
	// read/recv/readv - �����׽�������
	// read �����ݴ��뵽һ������
	// readv ������ݴ��뵽������飬Ч�ʸ���һЩ
	struct iovec vec[2];
	int writeable = bufferWriteableSize(buffer);
	//��ʼ������Ԫ��
	vec[0].iov_base = buffer->data + buffer->writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);// 40k����
	vec[1].iov_base = buffer->data + buffer->writePos;
	vec[1].iov_len = 40960;
	//��������
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
		buffer->writePos = buffer->capacity;// buffer��д�����Ż����
		bufferAppendData(buffer, tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}
// ����"\r\n"ȡ��һ�� �ҵ��������ݿ��е�λ�ã����ظĵ�ַ
char* bufferFindCRLF(struct Buffer* buffer)
{
	// strstr --> ���ַ�����ƥ�����ַ���(����\0����)
	// ��haystack��ʼ��Ѱ��needle,�������ҿ�ʼѰ��
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> �����ݿ���ƥ�������ݿ�(��Ҫָ�����ݿ��С)
	//void *memmem(const void *haystack, size_t haystacklen,
	//const void* needle, size_t needlelen);

	char * ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
	return ptr;
}
// TODO ����sendfile���ͷ�ʽ���ж��ļ�����Ŀ¼
int bufferSendData(struct Buffer* buffer, int socket)
{
	// �ж�buffer�����Ƿ�����Ҫ���͵����� �õ�δ�����ݼ�������
	int readable = bufferReadableSize(buffer); 
	if (readable > 0)
	{
		//���ͳ�ȥbuffer->data + buffer->readPos ��������λ��+�Ѿ�������λ��
		// �ܵ����� -- �����Ѿ��Ͽ������������������ݣ����ֹܵ����� -- TCPЭ��
		// ���ں˲����ź�ʱ��MSG_NOSIGNAL���ԣ�������������
		// Linux���źż���ߣ�Linux������źŶ�����ֹ�ź�
		int count = send(socket, buffer->data + buffer->readPos, readable,MSG_NOSIGNAL);
		if (count > 0)
		{
			// �����ƶ�δ��������λ��
			buffer->readPos += count;
			// ��΢����һ��
			usleep(1); // 1΢��
		}
		return count;
	}
	return 0;
}
