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

PUBLIC int printf(const char* fmt, ...){//因为用户进程工作在用户态，没有操作io的权限，所以需要系统调用
	int i;
	char buf[STR_DEFAULT_LEN];
	va_list arg = (va_list)((char*)(&fmt)+4);
	i = vsprintf(buf, fmt, arg);
	int c = write(1, buf, i);
	assert(c == i);
	return i;
}

PUBLIC int printl(const char *fmt, ...){
	int i;
	char bufpSTRDEFAULT_LEN[;
	va_list arg = (va_list)((char*)(&fmt) + 4);
	i = vsprintf(buf, fmt, arg);
	printx(buf);
	return i;
}
