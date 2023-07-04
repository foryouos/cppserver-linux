#pragma once
#include <string>
using namespace std;
// ��д�ڴ�ṹ�壬����ַ����������׽������ݣ���д���������ݷ���
class Buffer
{
public:
	Buffer(int size);  // ��ʼ��Buffer,size�����С,�ɸ���Buffer��дָ�������ز���
	~Buffer();        // �����ڴ溯��
	
	// Buffer���ݺ���,sizeʵ����Ҫ�Ĵ�С
	void extendRoom(int size);
	// �õ�ʣ���д���ڴ�����
	inline int writeableSize()
	{
		return m_capacity - m_readPos;
	};
	// �õ�ʣ��ɶ����ڴ�����
	inline int readableSize()
	{
		return m_writePos - m_readPos;
	};
	// д�ڴ� 1��ֱ��д��2�������׽�������
	// buffer ,��ӵ��ַ������ַ���data��Ӧ�ĳ���,��data��ָ������ݴ��뵽buffer��ȥ
	int appendString(const char* data, int size);
	// ����ַ�����������
	int appendString(const char* data);
	int appendString(const string data);
	//2�������׽������ݶ����� ���ؽ������ݴ�С
	int socketRead(int fd);
	// �ҵ��������ݿ��е�λ�ã����ظĵ�ַ���� �ո��� ȡ��һ��  ����\r\n
	char* findCRLF();
	// �������� ���������ļ�������
	int sendData( int socket);
	// ʹ��Linux��sendfile�����ļ�
	int sendData(int cfd, int fd, off_t offset, int size);
	//�õ������ݵ���ʼλ��
	inline char* data()
	{
		return m_data + m_readPos;
	}
	// �ƶ�readPosλ��+count
	inline int readPosIncrease(int count)
	{
		m_readPos += count;
		return m_readPos;
	}
private:
	//ָ���ڴ��ָ��
	char* m_data; 
	int m_capacity; //�ڴ�����
	int m_readPos = 0;  //����ַ��¼��ʲôλ�ÿ�ʼ������
	int m_writePos = 0; //д��ַ����¼��ʲôλ��д����
};

