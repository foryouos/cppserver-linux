#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include "Channel.h"
//��Ҫ���ݳ�Ա��������
//���������ҵ���Ӧchannel���Լ���Ӧ�Ķ�д�ļ��������ص�����
struct ChannelMap
{
	int size; //��¼ָ��ָ�������Ԫ�ظ���
	// struct Channel* list[]
	struct Channel** list; //����ָ�룬ָ���Լ�ָ������飬��ָ������ڴ�
};

// ��ʼ�������ṹ������ڴ�
struct ChannelMap* channelMapInit(int size);

//���map���ݺ���
void ChannelMapClear(struct ChannelMap* map);

// ������Ϊ��̬���飬�ռ䲻������Ҫ����
//map��Ӧ�Ǹ�map�������ݣ�newSize �µ��ڴ��С��unitSizeÿ����Ԫ��ռ�ֽ�
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);