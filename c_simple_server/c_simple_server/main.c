#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "server.h"
#include <stdio.h>
//��ʼ���������׽���

// argc ��������ĸ���
// argv[0]��ִ�г�������� 
// argv[1]����ĵ�һ�������� port
// argv[2]����ĵڶ�������   path

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = (unsigned short)atoi(argv[1]);
    //�л��������ĸ�Ŀ¼������Ŀ¼��ǰĿ¼�л�������Ŀ¼
    chdir(argv[2]);
    //��ʼ���������׽��� 0~65535�˿� ������5000���£����ܻᱻռ��
    printf("�󶨵Ķ˿�:%d\n", port);
    int lfd = initListenFd(port);
    //�������������� epoll
    epollRun(lfd);



    return 0;
}