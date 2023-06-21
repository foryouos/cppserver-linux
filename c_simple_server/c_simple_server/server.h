#pragma once
//��ʼ���������׽���
int initListenFd(unsigned short port);
//����epoll
int epollRun(int lfd);
//�Ϳͻ��˽�������
void* acceptClient(void* arg);
// ����http�������Ϣ
void* recvHttpRequest(void* arg);
//����������
int parseRequestLine(const char* line, int cfd); //���������ݣ��Լ�����ͨ�ŵ��ļ�������
//�����ļ�
int sendFile(const char* filename, int cfd);
// ������Ӧͷ(״̬��+��Ӧͷ) 
// ͨ���ļ���������״̬�룬״̬��������Ӧͷ�����ͣ�����
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);
//�����ļ���׺���õ��ļ�����
const char* getFileType(const char* name);
//����Ŀ¼
int sendDir(const char* dirName, int cfd);
// ��ʮ������תΪ10����
int hexToDec(char c);
//�����ַ�����
void decodeMsg(char* to, char* from);
