#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "server.h"
#include <stdio.h>
//初始化监听的套接字

// argc 输入参数的个数
// argv[0]可执行程序的名称 
// argv[1]传入的第一个参数， port
// argv[2]传入的第二个参数   path

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = (unsigned short)atoi(argv[1]);
    //切换服务器的根目录，将根目录当前目录切换到其它目录
    chdir(argv[2]);
    //初始化监听的套接字 0~65535端口 尽量不5000以下，可能会被占用
    printf("绑定的端口:%d\n", port);
    int lfd = initListenFd(port);
    //启动服务器程序 epoll
    epollRun(lfd);



    return 0;
}