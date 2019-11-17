#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

typedef struct s_console
{
	u32	current_start_addr;
	u32	original_addr;	
	u32	v_mem_limit;
	u32	cursor;
}CONSOLE;

#define DEFAULT_CHAR_COLOR	0x07

#endif
