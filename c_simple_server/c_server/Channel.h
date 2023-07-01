#pragma once  //宏防止被重复包含
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef int(*handleFunc)(void* arg); //参数泛型，handleFunc函数名

//定义文件描述符的读写事件，使用枚举   自定义
enum FDEvent
{
	TimeOut = 0x01,       //十进制1，超时了 1
	ReadEvent = 0x02,    //十进制4        10
	WriteEvent = 0x04   //十进制4  二进制 100
};

//描述符了文件描述符和读写，和读写回调函数以及参数
struct Channel
{
	//文件描述符
	int fd;
	// 事件
	int events;   //读/写/读写
	//回调函数
	handleFunc readCallback;   //读回调
	handleFunc writeCallback;  //写回调
	handleFunc destoryCallback;  //销毁回调
	// 回调函数参数
	void* arg;
}Channel;
//初始化一个Channel
struct Channel* channelInit(int fd,int events,handleFunc readFunc,handleFunc writeFunc, handleFunc destoryFunc,void* arg);
//修改fd的写事件(检测 or 不检测)，flag是否为写事件
void writeEventEnable(struct Channel* channel, bool flag);

//判断时候需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);