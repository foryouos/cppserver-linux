#pragma once
#include "Dispatcher.h"
#include <thread>
#include "Channel.h"
#include <queue>   //����
#include <map>     //��ֵ
#include <mutex>   //������
using namespace std;
//����ýڵ���Channel�¼�����
enum class ElemType:char
{
	ADD,
	DELETE,
	MODIFY
};
//����������еĽڵ� ���ͣ��ļ���������Ϣ
struct ChannelElement
{
	ElemType type;       //��δ���ýڵ���Channel
	Channel* channel;   //�ļ���������Ϣ
};
class Dispatcher;  //�ȶ������������������໥��������³��ֵ���������������

// �������е��¼���������Ӧ��ģ�ͣ���������ļ�����������¼�,������񣬴����������
// ����dispatcher�е�����Ƴ����޸Ĳ���
// �洢���������m_taskQ  �洢fd�Ͷ�Ӧchannel��Ӧ��ϵ:m_channelmap
// ȫ�����߳�->ͬʱ�������߳�
class EventLoop 
{
public:
	EventLoop();
	EventLoop(const string threadName);
	~EventLoop();
	//������Ӧ��ģ��
	int Run();
	// ����֮��ͻ����һЩ�ļ���������Ҫ����
	// ��������ļ�������fd,�ͼ�����¼�
	int eventActive(int fd, int event);
	// �������������� �����������п��ܴ���ͬʱ���ʣ��ӻ�����
	int AddTask(Channel* channel, ElemType type);
	// ������������е�����
	int ProcessTaskQ();
	// ��� ����dispatcher�еĽڵ�
	// ������ڵ��е�������ӵ�dispatcher��Ӧ�ļ�⼯������
	int Add(Channel* channel);
	// �Ƴ�  ����dispatcher�еĽڵ�
	int Remove(Channel* channel);
	// �޸� ����dispatcher�еĽڵ�
	int Modify(Channel* channel);
	// �ͷ�channel��Ҫ��Դ�ͷ�channel �ص��ļ�����������ַ���ڴ��ͷţ�channel��dispatcher�Ĺ�ϵ��Ҫɾ��
	int freeChannel(Channel* channel);
	static int readlocalMessage(void* arg);
	int readMessage();
	// �����߳�ID
	inline thread::id getTHreadID()
	{
		return m_threadID;
	}
	inline string getThreadName()
	{
		return m_threadName;
	}
private:
	void taskWakeup();
private:
	//���뿪�� EventLoop�Ƿ���
	bool m_isQuit;
	//��ָ��ָ��֮���ʵ��epoll,poll,select
	Dispatcher* m_dispatcher; 
	//������У��洢���񣬱���������оͿ����޸�dispatcher�����ļ�������
	//�������
	queue<ChannelElement*>m_taskQ;

	//map �ļ���������Channel֮��Ķ�Ӧ��ϵ  ͨ������ʵ��
	map<int,Channel*> m_channelmap;
	// �߳���أ��߳�ID��name
	thread::id m_threadID;
	string m_threadName;  //���߳�ֻ��һ�����̶����ƣ���ʼ��Ҫ��Ϊ����
	//�������������������
	mutex m_mutex;
	// ��������
	int m_socketPair[2]; //�洢����ͨ��fdͨ��socketpair��ʼ��
};

