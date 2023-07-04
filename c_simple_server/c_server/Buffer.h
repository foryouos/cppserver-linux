#pragma once
// ��д�ڴ�ṹ��
struct Buffer
{
	//ָ���ڴ��ָ��
	char* data;
	int capacity; //�ڴ�����
	int readPos;  //����ַ��¼��ʲôλ�ÿ�ʼ������
	int writePos; //д��ַ����¼��ʲôλ��д����
};

// ��ʼ��Buffer,size�����С,�ɸ���Buffer��дָ�������ز���
struct Buffer* bufferInit(int size);
// �����ڴ溯��
void bufferDestory(struct Buffer* buf);
// Buffer���ݺ���,sizeʵ����Ҫ�Ĵ�С
void bufferExtendRoom(struct Buffer* buffer, int size);
// �õ�ʣ���д���ڴ�����
int bufferWriteableSize(struct Buffer* buffer);
// �õ�ʣ��ɶ����ڴ�����
int bufferReadableSize(struct Buffer* buffer);
// д�ڴ� 1��ֱ��д��2�������׽�������
// buffer ,��ӵ��ַ������ַ���data��Ӧ�ĳ���,��data��ָ������ݴ��뵽buffer��ȥ
int bufferAppendData(struct Buffer* buffer,const char* data,int size);
// ����ַ�����������
int bufferAppendString(struct Buffer* buffer, const char* data);
//2�������׽������ݶ����� ���ؽ������ݴ�С
int bufferSocketRead(struct Buffer* buffer, int fd);
// �ҵ��������ݿ��е�λ�ã����ظĵ�ַ���ݿո���ȡ��һ�� 
char* bufferFindCRLF(struct Buffer* buffer);
// �������� ���������ļ�������
int bufferSendData(struct Buffer* buffer, int socket);
// ʹ��Linux��sendfile�����ļ�
int bufferSendFileData(int socket,int fd,int offset,int size);