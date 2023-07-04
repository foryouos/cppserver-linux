//#define _GNU_SOURCE 
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <ctype.h>
#include <errno.h>


Buffer::Buffer(int size) :m_capacity(size)
{
	// ��ʼ�����ִ�е������Ч
	m_data = (char*)malloc(size);
	bzero(m_data, size);
}

Buffer::~Buffer()
{
	if (m_data != nullptr)
	{
		free(m_data);
	}
}
void Buffer::extendRoom(int size)
{
	//1����д�ڴ湻�� ����Ҫ����
	if (writeableSize() >= size)
	{
		return;
	}
	//2,�ڴ���Ҫ�ϲ��Ź��ã�����Ҫ����
	// �ϲ�ʣ��Ŀ�д�ڴ��Լ��Ѷ��ڴ� > size
	else if (m_readPos + writeableSize() >= size)
	{
		//�õ�δ�����ڴ��С
		int readable = readableSize();
		// �ƶ��ڴ�,�ƶ�λ�ã��ƶ������ݵ�ַ�����ݿ��С
		memcpy(m_data, m_data + m_readPos, readable);
		// ����λ��
		m_readPos = 0;
		m_writePos = readable;
	}
	//3���ڴ治���ã�������
	else
	{
		void* temp = realloc(m_data, m_capacity + static_cast<size_t>(size));
		//
		if (temp == NULL)
		{
			return; //ʧ����
		}
		//����չ���ڴ���г�ʼ��temp+buffer->capacity.capacity�Ѿ���ʼ������
		memset((char*)temp + m_capacity, 0, size);
		// ��������
		m_data = static_cast<char*>(temp);
		m_capacity += size;
	}
}


int Buffer::appendString(const char* data, int size)
{
	//�ж�buffer��data�ǲ���ָ�����Ч�ڴ�
	if (data == nullptr || size <= 0)
	{
		return -1;
	}
	// ����,���Ը�����Ҫ�Զ���ѡ���Ƿ���Ҫ����
	extendRoom(size);
	// ���ݿ��� buffer->data + buffer->writePos��д������
	memcpy(m_data + m_writePos, data, size);
	m_writePos += size;
	return 0;
}

int Buffer::appendString(const char* data)
{
	int size = strlen(data);
	int ret = appendString(data, size);
	return ret;
}

int Buffer::appendString(const string data)
{
	int ret = appendString(data.data());
	return ret;
}
int Buffer::socketRead(int fd)
{
	// read/recv/readv - �����׽�������
	// read �����ݴ��뵽һ������
	// readv ������ݴ��뵽������飬Ч�ʸ���һЩ
	struct iovec vec[2];
	int writeable = writeableSize();
	//��ʼ������Ԫ��
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);// 40k����
	vec[1].iov_base = m_data + m_writePos;
	vec[1].iov_len = 40960;
	//��������
	int result = readv(fd, vec, 2);
	if (result == -1)
	{
		return -1;
	}
	else if (result <= writeable)
	{
		m_writePos += result;
	}
	else
	{
		m_writePos = m_capacity;// buffer��д�����Ż����
		appendString(tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* Buffer::findCRLF()
{
	// strstr --> ���ַ�����ƥ�����ַ���(����\0����)
	// ��haystack��ʼ��Ѱ��needle,�������ҿ�ʼѰ��
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> �����ݿ���ƥ�������ݿ�(��Ҫָ�����ݿ��С)
	//void *memmem(const void *haystack, size_t haystacklen,
	//const void* needle, size_t needlelen);

	char* ptr = static_cast<char*>(memmem(m_data + m_readPos, readableSize(), "\r\n", 2));
	return ptr;
}

int Buffer::sendData(int socket)
{
	// �ж�buffer�����Ƿ�����Ҫ���͵����� �õ�δ�����ݼ�������
	int readable = readableSize();
	if (readable > 0)
	{
		//���ͳ�ȥbuffer->data + buffer->readPos ��������λ��+�Ѿ�������λ��
		// �ܵ����� -- �����Ѿ��Ͽ������������������ݣ����ֹܵ����� -- TCPЭ��
		// ���ں˲����ź�ʱ��MSG_NOSIGNAL���ԣ�������������
		// Linux���źż���ߣ�Linux������źŶ�����ֹ�ź�
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count > 0)
		{
			// �����ƶ�δ��������λ��
			m_readPos += count;
			// ��΢����һ��
			usleep(1); // 1΢��
		}
		return count;
	}
	return 0;
}

int Buffer::sendData(int cfd, int fd, off_t offset, int size)
{
	int count = 0;
	while (offset < size)
	{
		//ϵͳ�����������ļ���linux�ں��ṩ��sendfile Ҳ�ܼ��ٿ�������
		// sendfile�����ļ�Ч�ʸߣ����ļ�Ŀ¼ʹ��send
		//ͨ���ļ������������ļ���������fd��Ӧ���ļ�ƫ����һ��Ϊ�գ�
		//�������ļ����ַ��Ͳ�ȫ��offset���Զ��޸ĵ�ǰ��ȡλ��
		int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));
		if (ret == -1 && errno == EAGAIN)
		{
			printf("not data ....");
			perror("sendfile");
		}
		count += (int)offset;
	}
	return count;
}
