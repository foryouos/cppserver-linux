#pragma once
#include <stdarg.h>
#include <stdio.h>
// 0为删除日志退出
// 1启动日志输出
#define  DEBUG   1  
#if DEBUG
/*
*  如果不加 do ... while(0) 在进行条件判断的时候(只有一句话), 省略了{}, 就会出现语法错误
*  if
*     xxxxx
*  else
*     xxxxx
*  宏被替换之后, 在 else 前面会出现一个 ;  --> 语法错误
*/
#define LOG(type, fmt, args...)  \
  do{\
    printf("%s: %s@%s, line: %d\n***LogInfo[", type, __FILE__, __FUNCTION__, __LINE__);\
    printf(fmt, ##args);\
    printf("]\n\n");\
  }while(0)
#define Debug(fmt, args...) LOG("DEBUG", fmt, ##args)
#define Error(fmt, args...) do{LOG("ERROR", fmt, ##args);exit(0);}while(0)
#else
#define LOG(fmt, args...)  
#define Debug(fmt, args...)
#define Error(fmt, args...)
#endif