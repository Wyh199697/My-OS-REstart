#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proc.h"
#include "proto.h"
#include "string.h"

int vsprintf(char* buf, const char* fmt, va_list args){
	char* p;
	char tmp[256];
	va_list p_next_arg = args;

	for(p=buf; *fmt; fmt++){//*fmt是什么判断条件？(*fmt) != '\0'?
		if(*fmt != '%'){
			*p++ = *fmt; //printf在% 之外的部分也需要打印出来,这里是在检测是否有%部分
			continue;
		}

		fmt++;
		switch(*fmt){
			case 'x'://x就是把整型用16进制表示出来
				itoa(tmp, *((int*)p_next_arg));
				strcpy(p, tmp);
				p_next_arg += 4;
				p += strlen(tmp);
				break;
			case 's':
				break;
			default:
				break;
		}
	}
	return (p - buf);
}
