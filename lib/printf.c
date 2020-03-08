#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "hd.h"

int printf(const char* fmt, ...){//因为用户进程工作在用户态，没有操作io的权限，所以需要系统调用
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt)+4);
	i = vsprintf(buf, fmt, arg);
	buf[i] = 0;
	printx(buf);
	return i;
}
