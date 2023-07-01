#pragma once
#include "Dispatcher.h"
#include <stdbool.h>
#include "Channel.h"
#include "ChannelMap.h"
#include <pthread.h>
//��ͬ�ַ�ģ��ʵ��
extern struct Dispatcher EpollDispatcher;  
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;
//����ýڵ���Channel
enum ELemType
{
	ADD,
	DELETE,
	MODIFY
};


//����������еĽڵ� ���ͣ��ļ���������Ϣ������һ��ָ��
struct ChannelElement
{
	int type;                  //��δ���ýڵ���Channel
	struct Channel* channel;   //�ļ�������
	struct ChannelElement* next;
};


struct Dispatcher;  //�ȶ������������������໥��������³��ֵ���������������
// ��EventLoop�洢��һ��Dispatcher�洢���Ӧ��DispatcherData
// ͷ��㿪�أ������߳�
struct EventLoop 
{
	//���뿪�� EventLoop�Ƿ���
	bool isQuit;
	struct Dispatcher* dispatcher;  // �ַ�ģ��dispatcherʵ�� select poll epoll������ʵ����ѡһ
	void* dispatcherData;    //����Ҫ������
	//������У��洢���񣬱���������оͿ����޸�dispatcher�����ļ�������
	// ����
	//ͷ����Ϊ�ڵ�
	struct ChannelElement* head;
	struct ChannelElement* tail;

	//map �ļ���������Channel֮��Ķ�Ӧ��ϵ  ͨ������ʵ��
	struct ChannelMap* channelmap;
	// �߳���أ��߳�ID��name
	pthread_t threadID;
	char threadName[32];  //���߳�ֻ��һ�����̶����ƣ���ʼ��Ҫ��Ϊ����
	//�������������������
	pthread_mutex_t mutex;
	// ��������
	int socketPair[2]; //�洢����ͨ��fdͨ��socketpair��ʼ��
};

//EventLoop ʵ����
struct EventLoop* eventLoopInit();
//���̴߳��߳����Ʋ���
struct EventLoop* eventLoopInitEx(const char* threadName);
//������Ӧ��ģ��
int eventLoopRun(struct EventLoop* evLoop);
// ����֮��ͻ����һЩ�ļ���������Ҫ����
// ��������ļ�������fd,�ͼ�����¼�
int eventActivate(struct EventLoop* evLoop,int fd,int event);
// �������������� �����������п��ܴ���ͬʱ���ʣ��ӻ�����
int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel,int type);
// ������������е�����
int eventLoopProcessTask(struct EventLoop* evLoop);

// ��� ����dispatcher�еĽڵ�
// ������ڵ��е�������ӵ�dispatcher��Ӧ�ļ�⼯������
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
// �Ƴ�  ����dispatcher�еĽڵ�
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
// �޸� ����dispatcher�еĽڵ�
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
// �ͷ�channel��Ҫ��Դ�ͷ�channel �ص��ļ�����������ַ���ڴ��ͷţ�channel��dispatcher�Ĺ�ϵ��Ҫɾ��
int destoryChannel(struct EventLoop* evLoop, struct Channel* channel);