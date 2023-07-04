#pragma once  //���ֹ���ظ�����
#include <functional>
using namespace std;
//�������ͣ�handleFunc������ ����ָ����ͨ��������������ľ�̬����
//using handleFunc = int(*)(void* ); 
//�����ļ��������Ķ�д�¼���ʹ��ö��   �Զ���
// C++11 ǿ����ö��
enum class FDEvent
{
	TimeOut = 0x01,       //ʮ����1����ʱ�� 1
	ReadEvent = 0x02,    //ʮ����2        10
	WriteEvent = 0x04   //ʮ����4  ������ 100
};
// �ɵ��ö����װ�����������ָ�룬�ɵ��ö���(��������һ��ʹ��)
// ���յõ��˵�ַ������û�е���


//���������ļ��������Ͷ�д���Ͷ�д�ص������Լ�����
class Channel
{
public:
	using handleFunc = function<int(void*)>;
	//��ʼ��һ��Channel
	Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void* arg);
	//�ص�����
	handleFunc readCallback;   //���ص�
	handleFunc writeCallback;  //д�ص�
	handleFunc destoryCallback;  //���ٻص�
	//�޸�fd��д�¼�(��� or �����)��flag�Ƿ�Ϊд�¼�
	void writeEventEnable(bool flag);
	//�ж�ʱ����Ҫ����ļ���������д�¼�
	bool isWriteEventEnable();
	// �����������������ò���Ҫѹջ��ֱ�ӽ��д�����滻����߳���ִ��Ч�ʣ���Ҳ��Ҫ�����ڴ棬�����ڼ�
	//ȡ�����ɳ�Ա��ֵ��ʹ�ô˷�����֤�˰�ȫ�ԣ�����ֱ�Ӷ�ȡ���������޸ģ�����ַ�Կ����޸�
	//ȡ���¼�
	inline int getEvent()
	{
		return m_events;
	}
	// ȡ��˽���ļ�������
	inline int getSocket()
	{
		return m_fd;
	}
	// ȡ���ص���������,����const֮���ַҲ�޷����ģ�ֻ��
	inline const void* getArg()
	{
		return m_arg;
	}
private:
	//�ļ�������
	int m_fd;
	// �¼�
	int m_events;   //��/д/��д
	// �ص���������
	void* m_arg;
};
