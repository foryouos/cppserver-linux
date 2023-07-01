#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include "Channel.h"
//主要数据成员就是数组
//根据数组找到对应channel，以及对应的读写文件描述符回调函数
struct ChannelMap
{
	int size; //记录指针指向数组的元素个数
	// struct Channel* list[]
	struct Channel** list; //二级指针，指向以及指针的数组，给指针分配内存
};

// 初始化，给结构体分配内存
struct ChannelMap* channelMapInit(int size);

//清空map数据函数
void ChannelMapClear(struct ChannelMap* map);

// 此数组为静态数组，空间不够，需要扩容
//map对应那个map进行扩容，newSize 新的内存大小，unitSize每个单元所占字节
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);