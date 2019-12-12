#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
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
		for(p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; ++p){
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
			for(p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++){
				p->ticks = p->priority;//如果所有进程ticks都为0，那么重置ticks
			}
		}
	}
}

PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m,  struct proc* p){
	assert(k_reenter == 0);/*make sure we are not in ring0??*/
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) || 
			src_dest == ANY || 
			src_dest == INTERRUPT);

	int ret = 0;
	int caller = proc2pid(p);
	MASSAGE* mla = (MASSAGE*)va2la(caller, m);
	mla->source = caller;

	assert(mla->source != src_dest);
	if(function == SEND){
		ret = msg_send(p, sec_dest, m);
		if(ret != 0){
			return ret;
		}else if(function == RECEIVE){
			ret = msgreceive(p, src_dest, m);
			if(ret != 0){
				return ret;
			}
		}else{
			panic("{sys_sendrec} invalid function: "
					"%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
		}
	}
	return 0;
}
