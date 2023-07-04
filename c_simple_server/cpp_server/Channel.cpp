#include "Channel.h"
#include <stdlib.h>

Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void* arg)
{
	//对结构体初始化
	m_arg = arg;  //保存了反映堆模型初始化数据
	m_fd = fd;
	m_events = (int)events;
	readCallback = readFunc;
	writeCallback = writeFunc;
	destoryCallback = destoryFunc;
}
//添加写属性
// 若对应为10 想要写添加写属性，与100异或，的110读写属性
// 如不写，第三位清零，若为110，第三位清零，将写取反011，在按位与& 010只留下读事件
void Channel::writeEventEnable(bool flag)
{
	if (flag) //如果为真，添加写属性
	{
		// 异或 相同为0 异为1
		// WriteEvent 从右往左数第三个标志位1，通过异或 让channel->events的第三位为1
		m_events |= static_cast<int>(FDEvent::WriteEvent); // 按位异或 int events整型32位，0/1,
	}
	else // 如果不写，让channel->events 对应的第三位清零
	{
		// ~WriteEvent 按位与， ~WriteEvent取反 011 然后与 channel->events按位与&运算 只有11 为 1，其它皆为0只有同为真时则真，一假则假，1为真，0为假
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent);  //channel->events 第三位清零之后，写事件就不再检测
	}
}
bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent);  //按位与 ，第三位都是1，则是写，如果成立，最后大于0，如果不成立，最后为0
}