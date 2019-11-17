#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"


/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	//disp_str("+");
	return ticks;
}

PUBLIC void schedule(){
	PROCESS* p;
	int greatest_ticks = 0;
	while(!greatest_ticks){
		for(p = proc_table; p < proc_table+NR_TASKS; ++p){
			if(p->ticks > greatest_ticks){//选出ticks最多的那个进程
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}
		/*if(p_proc_ready->ticks > 0){
			disp_str("<");
			disp_int(p_proc_ready->ticks);
			disp_str(" ");
			disp_int(p_proc_ready->pid);
			disp_str(">");
		}*/
		if(!greatest_ticks){
			for(p = proc_table; p < proc_table+NR_TASKS; p++){
				p->ticks = p->priority;//如果所有进程ticks都为0，那么重置ticks
			}
		}
	}
}
