#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
struct EventLoop* eventLoopInit() //调用带参数的初始化即可
{
    return eventLoopInitEx(NULL);
}
//写数据
void taskWakeup(struct EventLoop* evLoop)
{
    const char* msg = "唤醒线程";
    write(evLoop->socketPair[0], msg, strlen(msg));
}
// 读数据 接受evLoop->socketPair[0]发送过来的数据
int readlocalMessage(void* arg) 
{
    struct EventLoop* evLoop = (struct EventLoop*)arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf)); //目的仅为触发一次读事件，检测文件描述符解除阻塞
    return 0;
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    // 初始化结构体，并分配内存
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    // 刚开始evenloop没有运行
    evLoop->isQuit = false;
    evLoop->threadID = pthread_self(); //当前线程的ID
    //初始化互斥锁
    pthread_mutex_init(&evLoop->mutex, NULL);
    //子线程动态名字，主线程动态名字
    strcpy(evLoop->threadName , threadName== NULL ? "MainThread" : threadName);
    //
    evLoop->dispatcher = &EpollDispatcher; //选择模型
    evLoop->dispatcherData = evLoop->dispatcher->init();
    //链表
    evLoop->head = evLoop->tail = NULL;
    // map
    evLoop->channelmap = channelMapInit(128); //当申请的空间不够是，会对齐进行二倍初始化
    // 线程通信socketpair初始化 -- 同一线程数据间通信socketpair通信是两个，0发数据，通过1读出，反之相反
    // evLoop->socketPair传出参数，
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair); 
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
    // 指定规则:evLoop->socketPair[0] 发送数据，evLoop->socketPair[1]接受数据
    // 接受数据添加到dispatcher检测文件描述符的集合中  ,readlocalMessage读事件对应的回调函数
    struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readlocalMessage,NULL,NULL,evLoop);
    //channel 添加到任务队列
    eventLoopAddTask(evLoop, channel, ADD);

    return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop)
{
    //断言对指针判断
    assert(evLoop != NULL);
    //比较线程ID，当前线程ID与我们保存的线程ID是否相等
    if (evLoop->threadID != pthread_self())
    {
        //不相等时 直接返回-1
        return -1;
    }

    // 启动反应堆模型启动的主要是dispatcher，对应的模型
    // 取出时间分发和检测模型
    struct Dispatcher* dispatcher = evLoop->dispatcher;
    // 循环进行时间处理
    while (!evLoop->isQuit) //只要没有停止 死循环
    {
        dispatcher->dispatch(evLoop, 2); //在工作时的超时时长
        eventLoopProcessTask(evLoop);    //处理任务队列
    }
    return 0;
}

int eventLoopProcessTask(struct EventLoop* evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);
    // 取出头结点
    struct ChannelElement* head = evLoop->head;
    //遍历链表
    while (head != NULL)
    {
        //读链表中的Channel,根据Channel进行处理
        struct Channel* channel = head->channel;
        // 判断任务类型
        if (head->type == ADD)
        {
            // 需要channel里面的文件描述符evLoop里面的数据
            //添加  -- 每个功能对应一个任务函数，更利于维护
            eventLoopAdd(evLoop, channel);
        }
        else if (head->type == DELETE)
        {
            Debug("断开了连接");
            //删除
            eventLoopRemove(evLoop, channel);
            // 需要资源释放channel 关掉文件描述符，地址堆内存释放，channel和dispatcher的关系需要删除

        }
        else if (head->type == MODIFY)
        {
            //修改  的文件描述符事件
            eventLoopModify(evLoop, channel);
        }
        struct ChannelElement* tmp = head;
        head = head->next;
        free(tmp);
    }
    //处理完之后
    evLoop->head = evLoop->tail = NULL;
    //将处理后的task从当前链表中删除，(需要加锁)

    pthread_mutex_unlock(&evLoop->mutex);
    return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type)
{
    //加锁，有可能是当前线程，也有可能是主线程
    pthread_mutex_lock(&evLoop->mutex);
    // 创建新节点
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // node节点添加到任务队列里面
    // 链表为空
    if (evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        //如果不为空，将node放到链表尾部,尾部指向node，让后尾部后移
        evLoop->tail->next = node; //添加
        evLoop->tail = node;       //后移
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // 处理节点
    /*
    * 如当前EventLoop反应堆属于子线程
    *   1，对于链表节点的添加：可能是当前线程也可能是其它线程(主线程)
    *       1),修改fd的事件，可能是当前线程发起的，还是当前子线程进行处理
    *       2),添加新的fd，和新的客户端发起连接，添加任务节点的操作由主线程发起
    *   2，主线程只负责和客户端建立连接，判断当前线程，不让主线程进行处理，分给子线程
    *       不能让主线程处理任务队列，需要由当前的子线程处理
    */
    if (evLoop->threadID == pthread_self())
    {
        //当前子线程
        // 直接处理任务队列中的任务
        eventLoopProcessTask(evLoop);
    }
    else
    {
        //主线程 -- 告诉子线程处理任务队列中的任务
        // 1,子线程在工作 2，子线程被阻塞了：1，select,poll,epoll,如何解除其阻塞，在本代码阻塞时长是2s
        // 在检测集合中添加属于自己(额外)的文件描述，不负责套接字通信，目的控制文件描述符什么时候有数据,辅助解除阻塞
        // 满足条件，两个文件描述符，可以相互通信，//1，使用pipe进程间通信，进程更可，//2，socketpair 文件描述符进行通信
        taskWakeup(evLoop); //主线程调用，相当于向socket添加了数据
    }


    return 0;
}

int eventLoopAdd(struct EventLoop* evLoop,struct Channel* channel)
{
    //把任务节点中的任务添加到dispatcher对应的检测集合里面，
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelmap; //channel和文件描述符的关系
    //如果当前文件描述符大于channelMap对应关系中的size，channelMap空间不够，需要扩充
    if (fd >= channelMap->size)
    {
        if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*)))//fd新的扩容个数，
        {
            return -1; //如果扩容失败
        }
    }
    //找到fd对应数组元素的位置，并存储
    if (channelMap->list[fd] == NULL)
    {
        //把fd和channel对应关系存储起来
        channelMap->list[fd] = channel;  //将fd和channel的对应关系存储起来
        evLoop->dispatcher->add(channel, evLoop);  //加入
    }

    return 0;
}
int destoryChannel(struct EventLoop* evLoop, struct Channel* channel)
{
    // 删除channel 和fd的对应关系
    evLoop->channelmap->list[channel->fd] = NULL;
    // 关闭fd
    close(channel->fd);
    //释放channel  --复杂，里面节点之间复杂的关系

    free(channel);

    return 0;
}
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
    //调用dispatcher的remove函数进行删除
    // 将要删除的文件描述符
    int fd = channel->fd; 
    struct ChannelMap* channelMap = evLoop->channelmap; //channel和文件描述符的关系
    // 删除的fd不能大于size的最大，否则不再此存储过
    if (fd >= channelMap->size) //channelMap->size指针数组元素的总个数
    {
        
         return -1; //如果扩容失败
        
    }
    //从检测集合中删除 封装了poll,epoll select
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
{
    // 将要修改的文件描述符
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelmap; //channel和文件描述符的关系
    // TODO判断
    if (fd >= channelMap->size || channelMap->list[fd] == NULL) //channelMap->size指针数组元素的总个数
    {

        return -1; //如果扩容失败

    }
    //从检测集合中删除
    int ret = evLoop->dispatcher->modify(channel, evLoop);
    return ret;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
    // 判断函数传入的参数是否为有效
    if (fd < 0 || evLoop == NULL)
    {
        return -1;
    }
    //基于fd从EventLoop取出对应的Channel
    struct Channel* channel = evLoop->channelmap->list[fd]; //channelmap根据对应的fd取出对应的channel
    // 判断取出channel的fd与当前的fd是否相同
    assert(channel->fd == fd); //如果为假，打印出报错信息
    if (event & ReadEvent && channel->readCallback) //channel->readCallback不等于空
    {
        //调用channel的读回调函数
        channel->readCallback(channel->arg);
    }
    if (event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }

    return 0;
}
