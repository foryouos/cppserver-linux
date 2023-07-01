#include "ChannelMap.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
struct ChannelMap* channelMapInit(int size)
{
	//分配一块内存
	struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	//给map内部指针分配内存，大小为size
	map->size = size;
	map->list = (struct Channel**)malloc(sizeof(struct Channel*) * size);
	return map;
}

void ChannelMapClear(struct ChannelMap* map)
{
	//清空数组对应所有资源
	if (map != NULL)
	{
		//遍历数组
		for (int i = 0; i < map->size; ++i)
		{
			//如果对应的空间不为空，释放对应的空间资源
			if (map->list[i] != NULL)
			{
				free(map->list[i]);
			}
		}
		//释放数组
		free(map->list); //对应的内存地址
		map->list = NULL;
	}
	//释放后大小为0
	map->size = 0;
}

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize)
{
	//只有map->size < newSize时才需要做扩容操作
	if (map->size < newSize)
	{
		int curSize = map->size; //取出当前的size大小
		// 容量每次扩大原来的一倍,循环扩大，直到大于newSize
		while (curSize < newSize)
		{
			curSize *= 2;
		}
		// 扩容 realloc 函数,给map->list指针指向的内存地址，重新分配一块更大的内存，新的内存大小curSize * unitSize
		// 在分配新内存时，从原来的地址分配，如果后面没有足够的内存，realloc会重新分配一块更大内存，将原有的数据复制过去
		// 内存地址发生变化，需要重新赋值
		// 定义新的指针，将realloc的返回值保存起来，做判断是不是有效地址，
		// 若是有效地址，就把有效地址给map对应的list指针，若无效，重新分配内存失败
		struct Channel** temp = realloc(map->list, curSize * unitSize); // 返回分配成功之后，数据块的起始地址
		if (temp == NULL)
		{
			return false;  //
		}
		map->list = temp; //重新赋值
		
		// 对扩展出来的地址进行初始化
		memset(&map->list[map->size], 0, (curSize - map->size)*unitSize);

		map->size = curSize; //更新数组容量
		

	}
	return true;
}
