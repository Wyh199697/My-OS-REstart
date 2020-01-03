#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "global.h"
#include "keyboard.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY*p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

PUBLIC void task_tty(){
	//assert(0);
	TTY* p_tty;
	init_keyboard();

	for(p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++){
		init_tty(p_tty);
	}

	select_console(0);

	while(1){
		for(p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++){
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

PRIVATE void init_tty(TTY* p_tty){
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	init_screen(p_tty);
}

PRIVATE void tty_do_read(TTY* p_tty){
	if(is_current_console(p_tty->p_console)){
		keyboard_read(p_tty);
	}
}

PRIVATE void tty_do_write(TTY* p_tty){
	if(p_tty->inbuf_count > 0){
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES){
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}

PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
			put_key(p_tty, key);
        }
        else {
                int raw_code = key & MASK_RAW;
                switch(raw_code) {
				case ENTER:
					put_key(p_tty, '\n');
					break;
				case BACKSPACE:
					put_key(p_tty, '\b');
					break;
                case UP:
                    if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){
						scroll_screen(p_tty->p_console, SCR_DN);
					}
					break;
				case DOWN:
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)){
						scroll_screen(p_tty->p_console, SCR_UP);
					}
					break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
			/* Alt + F1~F12 */
			if ((key & FLAG_CTRL_L) || (key & FLAG_CTRL_R)) {
				select_console(raw_code - F1);
			}
			break;
                default:
                        break;
                }
        }
}

PRIVATE void put_key(TTY* p_tty, u32 key){
	if(p_tty->inbuf_count < TTY_IN_BYTES){
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if(p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES){
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}

PUBLIC void tty_write(TTY* p_tty, char* buf, int len){
	char* p = buf;
	int i = len;
	while(i){
		out_char(p_tty->p_console, *p++);
		i--;
	}
}

PUBLIC int sys_printx(int _unused1, int _unused2, char* s, struct proc* p_proc){
	const char* p;
	char ch;
	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;
	//没明白这样处理的意义内核态和用户态为什么字符串地址不同？有可能是因为防止任务和进程的段基址不同，这个说法的前提是ring0的段基址等于0，当用户态调用的时候用va2la函数算出的线性地址就是ring0的偏移地址。ring0段基址应该总是为0
	if(k_reenter == 0){ //用户态调用系统调用会使k_reenter=0
		p = va2la(proc2pid(p_proc), s);
	}else if(k_reenter > 0){ //内核态调用系统调用会使k_reenter>0
		p = s;
	}else{
		p = reenter_err;
	}
	if((*p == MAG_CH_PANIC) || (*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS])){
		disable_int();
		char* v = (char*)V_MEM_BASE;
		const char * q = p + 1;
		while(v < (char*)(V_MEM_BASE + V_MEM_SIZE)){
			*v++ = *q++;
			*v++ = RED_CHAR;
			if(!*q){
				while(((int)v - V_MEM_BASE) % (SCREEN_WIDTH * 16)){
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}
		__asm__ __volatile__("hlt");
	}
	while((ch = *p++) != 0){
		if(ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT){
			continue;
		}
		out_char(tty_table[p_proc->nr_tty].p_console, ch);
	}
	return 0;
}
