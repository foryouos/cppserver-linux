#pragma once
#include "Channel.h"
#include <string>
using namespace std;
//Dispatcher 结构体
class EventLoop;  //先定义出，避免编译器在相互包含情况下出现蛋生鸡鸡生蛋问题
//Epoll,Poll,Select模型
class Dispatcher
{

public:
	Dispatcher(struct EventLoop* evLoop);
	virtual ~Dispatcher();  //也虚函数，在多态时
	virtual int add();   //等于 =纯虚函数，就不用定义
	//删除 将某一个节点从epoll树上删除
	virtual int remove();
	//修改
	virtual int modify();
	//事件检测， 用于检测待检测三者之一模型epoll_wait等的一系列事件上是否有事件被激活，读/写事件
	virtual int dispatch(int timeout = 2);//单位 S 超时时长
	inline void setChannel(Channel* channel)
	{
		m_channel = channel;
	}
protected: // 访问权限受保护的，不能被外部访问 ，但可以被子类继承，
	string m_name = string(); //为此实例定义名字
	Channel* m_channel;
	EventLoop* m_evLoop;
};