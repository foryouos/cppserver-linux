#pragma once
#include "Channel.h"

//Dispatcher 结构体
struct EventLoop;  //先定义出，避免编译器在相互包含情况下出现蛋生鸡鸡生蛋问题
struct Dispatcher
{
	//init 初始化epoll，poll，select所需要的数据块，返回不同的数据库类型，
	// 使用泛型 转换成对应的类型
	void* (*init)();  //poll poll_fd类型 /epoll epoll_event //select fd_set 所初始化的内容
	//添加 把待检测的文件描述符添加到对应的类型epoll/poll/select上面
	// 通过EventLoop就可以取出Dispatcher
	 //ChannelIO多路模型
	int (*add)(struct Channel* channel,struct EventLoop* evLoop); 
	//删除 将某一个节点从epoll树上删除
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	//修改
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	//事件检测， 用于检测待检测三者之一模型epoll_wait等的一系列事件上是否有事件被激活，读/写事件
	int (*dispatch)(struct EventLoop* evLoop,int timeout);//单位 S 超时时长
	//清除数据(关闭fd或者释放内存)
	int (*clear)(struct EventLoop* evLoop);
};