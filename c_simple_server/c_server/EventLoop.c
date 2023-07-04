#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
struct EventLoop* eventLoopInit() //���ô������ĳ�ʼ������
{
    return eventLoopInitEx(NULL);
}
//д����
void taskWakeup(struct EventLoop* evLoop)
{
    const char* msg = "�����߳�";
    write(evLoop->socketPair[0], msg, strlen(msg));
}
// ������ ����evLoop->socketPair[0]���͹���������
int readlocalMessage(void* arg) 
{
    struct EventLoop* evLoop = (struct EventLoop*)arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf)); //Ŀ�Ľ�Ϊ����һ�ζ��¼�������ļ��������������
    return 0;
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    // ��ʼ���ṹ�壬�������ڴ�
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    // �տ�ʼevenloopû������
    evLoop->isQuit = false;
    evLoop->threadID = pthread_self(); //��ǰ�̵߳�ID
    //��ʼ��������
    pthread_mutex_init(&evLoop->mutex, NULL);
    //���̶߳�̬���֣����̶߳�̬����
    strcpy(evLoop->threadName , threadName== NULL ? "MainThread" : threadName);
    //
    evLoop->dispatcher = &EpollDispatcher; //ѡ��ģ��
    evLoop->dispatcherData = evLoop->dispatcher->init();
    //����
    evLoop->head = evLoop->tail = NULL;
    // map
    evLoop->channelmap = channelMapInit(128); //������Ŀռ䲻���ǣ��������ж�����ʼ��
    // �߳�ͨ��socketpair��ʼ�� -- ͬһ�߳����ݼ�ͨ��socketpairͨ����������0�����ݣ�ͨ��1��������֮�෴
    // evLoop->socketPair����������
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair); 
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
    // ָ������:evLoop->socketPair[0] �������ݣ�evLoop->socketPair[1]��������
    // ����������ӵ�dispatcher����ļ��������ļ�����  ,readlocalMessage���¼���Ӧ�Ļص�����
    struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readlocalMessage,NULL,NULL,evLoop);
    //channel ��ӵ��������
    eventLoopAddTask(evLoop, channel, ADD);

    return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop)
{
    //���Զ�ָ���ж�
    assert(evLoop != NULL);
    //�Ƚ��߳�ID����ǰ�߳�ID�����Ǳ�����߳�ID�Ƿ����
    if (evLoop->threadID != pthread_self())
    {
        //�����ʱ ֱ�ӷ���-1
        return -1;
    }

    // ������Ӧ��ģ����������Ҫ��dispatcher����Ӧ��ģ��
    // ȡ��ʱ��ַ��ͼ��ģ��
    struct Dispatcher* dispatcher = evLoop->dispatcher;
    // ѭ������ʱ�䴦��
    while (!evLoop->isQuit) //ֻҪû��ֹͣ ��ѭ��
    {
        dispatcher->dispatch(evLoop, 2); //�ڹ���ʱ�ĳ�ʱʱ��
        eventLoopProcessTask(evLoop);    //�����������
    }
    return 0;
}

int eventLoopProcessTask(struct EventLoop* evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);
    // ȡ��ͷ���
    struct ChannelElement* head = evLoop->head;
    //��������
    while (head != NULL)
    {
        //�������е�Channel,����Channel���д���
        struct Channel* channel = head->channel;
        // �ж���������
        if (head->type == ADD)
        {
            // ��Ҫchannel������ļ�������evLoop���������
            //���  -- ÿ�����ܶ�Ӧһ����������������ά��
            eventLoopAdd(evLoop, channel);
        }
        else if (head->type == DELETE)
        {
            Debug("�Ͽ�������");
            //ɾ��
            eventLoopRemove(evLoop, channel);
            // ��Ҫ��Դ�ͷ�channel �ص��ļ�����������ַ���ڴ��ͷţ�channel��dispatcher�Ĺ�ϵ��Ҫɾ��

        }
        else if (head->type == MODIFY)
        {
            //�޸�  ���ļ��������¼�
            eventLoopModify(evLoop, channel);
        }
        struct ChannelElement* tmp = head;
        head = head->next;
        free(tmp);
    }
    //������֮��
    evLoop->head = evLoop->tail = NULL;
    //��������task�ӵ�ǰ������ɾ����(��Ҫ����)

    pthread_mutex_unlock(&evLoop->mutex);
    return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type)
{
    //�������п����ǵ�ǰ�̣߳�Ҳ�п��������߳�
    pthread_mutex_lock(&evLoop->mutex);
    // �����½ڵ�
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // node�ڵ���ӵ������������
    // ����Ϊ��
    if (evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        //�����Ϊ�գ���node�ŵ�����β��,β��ָ��node���ú�β������
        evLoop->tail->next = node; //���
        evLoop->tail = node;       //����
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // ����ڵ�
    /*
    * �統ǰEventLoop��Ӧ���������߳�
    *   1����������ڵ����ӣ������ǵ�ǰ�߳�Ҳ�����������߳�(���߳�)
    *       1),�޸�fd���¼��������ǵ�ǰ�̷߳���ģ����ǵ�ǰ���߳̽��д���
    *       2),����µ�fd�����µĿͻ��˷������ӣ��������ڵ�Ĳ��������̷߳���
    *   2�����߳�ֻ����Ϳͻ��˽������ӣ��жϵ�ǰ�̣߳��������߳̽��д����ָ����߳�
    *       ���������̴߳���������У���Ҫ�ɵ�ǰ�����̴߳���
    */
    if (evLoop->threadID == pthread_self())
    {
        //��ǰ���߳�
        // ֱ�Ӵ�����������е�����
        eventLoopProcessTask(evLoop);
    }
    else
    {
        //���߳� -- �������̴߳�����������е�����
        // 1,���߳��ڹ��� 2�����̱߳������ˣ�1��select,poll,epoll,��ν�����������ڱ���������ʱ����2s
        // �ڼ�⼯������������Լ�(����)���ļ��������������׽���ͨ�ţ�Ŀ�Ŀ����ļ�������ʲôʱ��������,�����������
        // ���������������ļ��������������໥ͨ�ţ�//1��ʹ��pipe���̼�ͨ�ţ����̸��ɣ�//2��socketpair �ļ�����������ͨ��
        taskWakeup(evLoop); //���̵߳��ã��൱����socket���������
    }


    return 0;
}

int eventLoopAdd(struct EventLoop* evLoop,struct Channel* channel)
{
    //������ڵ��е�������ӵ�dispatcher��Ӧ�ļ�⼯�����棬
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelmap; //channel���ļ��������Ĺ�ϵ
    //�����ǰ�ļ�����������channelMap��Ӧ��ϵ�е�size��channelMap�ռ䲻������Ҫ����
    if (fd >= channelMap->size)
    {
        if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*)))//fd�µ����ݸ�����
        {
            return -1; //�������ʧ��
        }
    }
    //�ҵ�fd��Ӧ����Ԫ�ص�λ�ã����洢
    if (channelMap->list[fd] == NULL)
    {
        //��fd��channel��Ӧ��ϵ�洢����
        channelMap->list[fd] = channel;  //��fd��channel�Ķ�Ӧ��ϵ�洢����
        evLoop->dispatcher->add(channel, evLoop);  //����
    }

    return 0;
}
int destoryChannel(struct EventLoop* evLoop, struct Channel* channel)
{
    // ɾ��channel ��fd�Ķ�Ӧ��ϵ
    evLoop->channelmap->list[channel->fd] = NULL;
    // �ر�fd
    close(channel->fd);
    //�ͷ�channel  --���ӣ�����ڵ�֮�临�ӵĹ�ϵ

    free(channel);

    return 0;
}
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
    //����dispatcher��remove��������ɾ��
    // ��Ҫɾ�����ļ�������
    int fd = channel->fd; 
    struct ChannelMap* channelMap = evLoop->channelmap; //channel���ļ��������Ĺ�ϵ
    // ɾ����fd���ܴ���size����󣬷����ٴ˴洢��
    if (fd >= channelMap->size) //channelMap->sizeָ������Ԫ�ص��ܸ���
    {
        
         return -1; //�������ʧ��
        
    }
    //�Ӽ�⼯����ɾ�� ��װ��poll,epoll select
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
{
    // ��Ҫ�޸ĵ��ļ�������
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelmap; //channel���ļ��������Ĺ�ϵ
    // TODO�ж�
    if (fd >= channelMap->size || channelMap->list[fd] == NULL) //channelMap->sizeָ������Ԫ�ص��ܸ���
    {

        return -1; //�������ʧ��

    }
    //�Ӽ�⼯����ɾ��
    int ret = evLoop->dispatcher->modify(channel, evLoop);
    return ret;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
    // �жϺ�������Ĳ����Ƿ�Ϊ��Ч
    if (fd < 0 || evLoop == NULL)
    {
        return -1;
    }
    //����fd��EventLoopȡ����Ӧ��Channel
    struct Channel* channel = evLoop->channelmap->list[fd]; //channelmap���ݶ�Ӧ��fdȡ����Ӧ��channel
    // �ж�ȡ��channel��fd�뵱ǰ��fd�Ƿ���ͬ
    assert(channel->fd == fd); //���Ϊ�٣���ӡ��������Ϣ
    if (event & ReadEvent && channel->readCallback) //channel->readCallback�����ڿ�
    {
        //����channel�Ķ��ص�����
        channel->readCallback(channel->arg);
    }
    if (event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }

    return 0;
}
