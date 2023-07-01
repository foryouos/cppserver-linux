#include "Channel.h"
#include <stdlib.h>
struct Channel* channelInit(int fd, int events, handleFunc readFunc,
	handleFunc writeFunc, handleFunc destoryFunc, void* arg)
{
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel)); //申请动态空间内存
	//对结构体初始化
	channel->arg = arg;  //保存了反映堆模型初始化数据
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readFunc;
	channel->writeCallback = writeFunc;
	channel->destoryCallback = destoryFunc;
	//返回初始化的结构体
	return channel;
}

void writeEventEnable(struct Channel* channel, bool flag)
{
	if (flag) //如果为真，添加写属性
	{
		// 异或 相同为0 异为1
		// WriteEvent 从右往左数第三个标志位1，通过异或 让channel->events的第三位为1
		channel->events |= WriteEvent; // 按位异或 int events整型32位，0/1,
	}
	else // 如果不写，让channel->events 对应的第三位清零
	{ 
		// ~WriteEvent 按位与， ~WriteEvent取反 011 然后与 channel->events按位与&运算 只有11 为 1，其它皆为0只有同为真时则真，一假则假，1为真，0为假
		channel->events = channel->events & ~WriteEvent;  //channel->events 第三位清零之后，写事件就不再检测
	} 

}

bool isWriteEventEnable(struct Channel* channel)
{

	return channel->events & WriteEvent;  //按位与 ，第三位都是1，则是写，如果成立，最后大于0，如果不成立，最后为0
}
