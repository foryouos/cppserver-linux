#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "TcpServer.h"
//��ʼ���������׽���

// argc ��������ĸ���
// argv[0]��ִ�г�������� 
// argv[1]����ĵ�һ�������� port
// argv[2]����ĵڶ�������   path

int main(int argc, char* argv[])
{
#if 0
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = (unsigned short)atoi(argv[1]);
    //�л��������ĸ�Ŀ¼������Ŀ¼��ǰĿ¼�л�������Ŀ¼
    chdir(argv[2]);
    // ����������
#else
    // VS code ����
    unsigned short port = 8080;
    chdir("/home/foryouos/blog");
#endif
    // ����������ʵ��
    struct TcpServer* server = tcpServerInit(port, 4);
    // ���������� - �����̳߳�-�Լ������׽��ֽ��з�װ�����ŵ����̵߳�������У�������Ӧ��ģ��
    // �ײ��epollҲ����������
    tcpServerRun(server);



    return 0;
}