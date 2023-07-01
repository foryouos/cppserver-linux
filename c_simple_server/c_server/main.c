#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "TcpServer.h"
//初始化监听的套接字

// argc 输入参数的个数
// argv[0]可执行程序的名称 
// argv[1]传入的第一个参数， port
// argv[2]传入的第二个参数   path

int main(int argc, char* argv[])
{
#if 0
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = (unsigned short)atoi(argv[1]);
    //切换服务器的根目录，将根目录当前目录切换到其它目录
    chdir(argv[2]);
    // 启动服务器
#else
    // VS code 调试
    unsigned short port = 8080;
    chdir("/home/foryouos/blog");
#endif
    // 创建服务器实例
    struct TcpServer* server = tcpServerInit(port, 4);
    // 服务器运行 - 启动线程池-对监听的套接字进行封装，并放到主线程的任务队列，启动反应堆模型
    // 底层的epoll也运行起来，
    tcpServerRun(server);



    return 0;
}