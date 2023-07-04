#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "SelectDispatcher.h"
#include "PollDispatcher.h"
#include "EpollDispatcher.h"
// ί�й��캯�����мǲ�Ҫ�ù��캯��֮���γ�һ���ջ� - ��ѭ��
EventLoop::EventLoop() : EventLoop(string())
{
}
EventLoop::EventLoop(const string threadName)
{
    // �տ�ʼevenloopû������
    m_isQuit = true;
    m_threadID = this_thread::get_id(); //��ǰ�̵߳�ID
    //���̶߳�̬���֣����̶߳�̬����
    m_threadName = m_threadName == string() ? "MainThread" : threadName;
    //
    m_dispatcher = new EpollDispatcher(this); //ѡ��ģ��

    m_channelmap.clear();  //��ղ���
    // �߳�ͨ��socketpair��ʼ��//evLoop->socketPair����������ͨ����������0�����ݣ�ͨ��1��������֮�෴
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
#if 0
    // ָ������:evLoop->socketPair[0] �������ݣ�evLoop->socketPair[1]��������
    // ����������ӵ�dispatcher����ļ��������ļ�����  ,readlocalMessage���¼���Ӧ�Ļص�����
    Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent,
        readlocalMessage, nullptr, nullptr, this);
#else
    // TODO ������ bind-������function��ĳ�Ա�����޷�����ĳ�Ա����ֱ�Ӵ��
    auto obj = bind(&EventLoop::readMessage, this);
    Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent,
        obj, nullptr, nullptr, this);
#endif
    //channel ��ӵ��������
    AddTask( channel, ElemType::ADD);

}

EventLoop::~EventLoop()
{
}

int EventLoop::Run()
{
    m_isQuit = false; //���˳�
    //�Ƚ��߳�ID����ǰ�߳�ID�����Ǳ�����߳�ID�Ƿ����
    if (m_threadID != this_thread::get_id())
    {
        //�����ʱ ֱ�ӷ���-1
        return -1;
    }
    // ѭ������ʱ�䴦��
    while (!m_isQuit) //ֻҪû��ֹͣ ��ѭ��
    {
        //���ó�ʼ��ʱѡ�е�ģ��Epoll,Poll��Select
        m_dispatcher->dispatch(); //�ڹ���ʱ�ĳ�ʱʱ��
        ProcessTaskQ();    //�����������
    }
    return 0;
}

int EventLoop::eventActive(int fd, int event)
{
    // �жϺ�������Ĳ����Ƿ�Ϊ��Ч
    if (fd < 0)
    {
        return -1;
    }
    //����fd��EventLoopȡ����Ӧ��Channel
    Channel* channel = m_channelmap[fd]; //channelmap���ݶ�Ӧ��fdȡ����Ӧ��channel
    // �ж�ȡ��channel��fd�뵱ǰ��fd�Ƿ���ͬ
    assert(channel->getSocket() == fd); //���Ϊ�٣���ӡ��������Ϣ
    if (event & (int)FDEvent::ReadEvent && channel->readCallback) //channel->readCallback�����ڿ�
    {
        //����channel�Ķ��ص�����
        channel->readCallback(const_cast<void*>(channel->getArg()));
    }
    if (event & (int)FDEvent::WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(const_cast<void*>(channel->getArg()));
    }
    return 0;
}

int EventLoop::AddTask(Channel* channel, ElemType type)
{
    //�������п����ǵ�ǰ�̣߳�Ҳ�п��������߳�
    m_mutex.lock();
    // �����½ڵ�
    ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    m_taskQ.push(node);
    m_mutex.unlock();
    // ����ڵ�
    /*
    * �統ǰEventLoop��Ӧ���������߳�
    *   1����������ڵ����ӣ������ǵ�ǰ�߳�Ҳ�����������߳�(���߳�)
    *       1),�޸�fd���¼��������ǵ�ǰ�̷߳���ģ����ǵ�ǰ���߳̽��д���
    *       2),����µ�fd�����µĿͻ��˷������ӣ��������ڵ�Ĳ��������̷߳���
    *   2�����߳�ֻ����Ϳͻ��˽������ӣ��жϵ�ǰ�̣߳��������߳̽��д����ָ����߳�
    *       ���������̴߳���������У���Ҫ�ɵ�ǰ�����̴߳���
    */
    if (m_threadID == this_thread::get_id())
    {
        //��ǰ���߳�
        // ֱ�Ӵ�����������е�����
        ProcessTaskQ();
    }
    else
    {
        //���߳� -- �������̴߳�����������е�����
        // 1,���߳��ڹ��� 2�����̱߳������ˣ�1��select,poll,epoll,��ν�����������ڱ���������ʱ����2s
        // �ڼ�⼯������������Լ�(����)���ļ��������������׽���ͨ�ţ�Ŀ�Ŀ����ļ�������ʲôʱ��������,�����������
        // ���������������ļ��������������໥ͨ�ţ�//1��ʹ��pipe���̼�ͨ�ţ����̸��ɣ�//2��socketpair �ļ�����������ͨ��
        taskWakeup(); //���̵߳��ã��൱����socket���������
    }
    return 0;
}

int EventLoop::ProcessTaskQ()
{
    //��������
    while (!m_taskQ.empty())
    {
        //��������task�ӵ�ǰ������ɾ����(��Ҫ����)
        // ȡ��ͷ���
        m_mutex.lock();
        ChannelElement* node = m_taskQ.front(); //��ͷ��
        m_taskQ.pop();  //��ͷ��㵯�����൱��ɾ�� 
        
        m_mutex.unlock();
        //�������е�Channel,����Channel���д���
        Channel* channel = node->channel;
        // �ж���������
        if (node->type == ElemType::ADD)
        {
            // ��Ҫchannel������ļ�������evLoop���������
            //���  -- ÿ�����ܶ�Ӧһ����������������ά��
            Add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {
            //Debug("�Ͽ�������");
            //ɾ��
            Remove(channel);
            // ��Ҫ��Դ�ͷ�channel �ص��ļ�����������ַ���ڴ��ͷţ�channel��dispatcher�Ĺ�ϵ��Ҫɾ��

        }
        else if (node->type == ElemType::MODIFY)
        {
            //�޸�  ���ļ��������¼�
            Modify(channel);
        }
        delete node;
    }
    return 0;
}

int EventLoop::Add(Channel* channel)
{
    //������ڵ��е�������ӵ�dispatcher��Ӧ�ļ�⼯�����棬
    int fd = channel->getSocket();
    //�ҵ�fd��Ӧ����Ԫ�ص�λ�ã����洢
    if (m_channelmap.find(fd) == m_channelmap.end())
    {
        m_channelmap.insert(make_pair(fd, channel)); //����ǰfd��channel��ӵ�map
        m_dispatcher->setChannel(channel); //���õ�ǰchannel
        int ret = m_dispatcher->add();  //����
        return ret;
    }
    return -1;
}

int EventLoop::Remove(Channel* channel)
{
    //����dispatcher��remove��������ɾ��
    // ��Ҫɾ�����ļ�������
    int fd = channel->getSocket();
    // �ж��ļ��������Ƿ��Ѿ��ڼ��ļ�����
    if (m_channelmap.find(fd) == m_channelmap.end())
    {
        return -1;
    }
    //�Ӽ�⼯����ɾ�� ��װ��poll,epoll select
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
    return ret;
}

int EventLoop::Modify(Channel* channel)
{
    // ��Ҫ�޸ĵ��ļ�������
    int fd = channel->getSocket();
    // TODO�ж�
    if (m_channelmap.find(fd) == m_channelmap.end()) 
    {
        return -1; 
    }
    //�Ӽ�⼯����ɾ��
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();
    return ret;
}

int EventLoop::freeChannel(Channel* channel)
{
    // ɾ��channel ��fd�Ķ�Ӧ��ϵ
    auto it = m_channelmap.find(channel->getSocket());
    if (it != m_channelmap.end())
    {

        m_channelmap.erase(it); //ɾ����Ӧ�ĵ�����
        // �ر�fd
        close(channel->getSocket());
        delete channel;
    }
    return 0;
}

int EventLoop::readlocalMessage(void* arg)
{
    EventLoop* evLoop = static_cast<EventLoop*>(arg);
    char buf[256];
    read(evLoop->m_socketPair[1], buf, sizeof(buf)); //Ŀ�Ľ�Ϊ����һ�ζ��¼�������ļ��������������
    return 0;
}

int EventLoop::readMessage()
{
    char buf[256];
    read(m_socketPair[1], buf, sizeof(buf)); //Ŀ�Ľ�Ϊ����һ�ζ��¼�������ļ��������������
    return 0;
}

void EventLoop::taskWakeup()
{
    const char* msg = "�����߳�";
    write(m_socketPair[0], msg, strlen(msg));
}
