#include "ChannelMap.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
struct ChannelMap* channelMapInit(int size)
{
	//����һ���ڴ�
	struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	//��map�ڲ�ָ������ڴ棬��СΪsize
	map->size = size;
	map->list = (struct Channel**)malloc(sizeof(struct Channel*) * size);
	return map;
}

void ChannelMapClear(struct ChannelMap* map)
{
	//��������Ӧ������Դ
	if (map != NULL)
	{
		//��������
		for (int i = 0; i < map->size; ++i)
		{
			//�����Ӧ�Ŀռ䲻Ϊ�գ��ͷŶ�Ӧ�Ŀռ���Դ
			if (map->list[i] != NULL)
			{
				free(map->list[i]);
			}
		}
		//�ͷ�����
		free(map->list); //��Ӧ���ڴ��ַ
		map->list = NULL;
	}
	//�ͷź��СΪ0
	map->size = 0;
}

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize)
{
	//ֻ��map->size < newSizeʱ����Ҫ�����ݲ���
	if (map->size < newSize)
	{
		int curSize = map->size; //ȡ����ǰ��size��С
		// ����ÿ������ԭ����һ��,ѭ������ֱ������newSize
		while (curSize < newSize)
		{
			curSize *= 2;
		}
		// ���� realloc ����,��map->listָ��ָ����ڴ��ַ�����·���һ�������ڴ棬�µ��ڴ��СcurSize * unitSize
		// �ڷ������ڴ�ʱ����ԭ���ĵ�ַ���䣬�������û���㹻���ڴ棬realloc�����·���һ������ڴ棬��ԭ�е����ݸ��ƹ�ȥ
		// �ڴ��ַ�����仯����Ҫ���¸�ֵ
		// �����µ�ָ�룬��realloc�ķ���ֵ�������������ж��ǲ�����Ч��ַ��
		// ������Ч��ַ���Ͱ���Ч��ַ��map��Ӧ��listָ�룬����Ч�����·����ڴ�ʧ��
		struct Channel** temp = realloc(map->list, curSize * unitSize); // ���ط���ɹ�֮�����ݿ����ʼ��ַ
		if (temp == NULL)
		{
			return false;  //
		}
		map->list = temp; //���¸�ֵ
		
		// ����չ�����ĵ�ַ���г�ʼ��
		memset(&map->list[map->size], 0, (curSize - map->size)*unitSize);

		map->size = curSize; //������������
		

	}
	return true;
}
