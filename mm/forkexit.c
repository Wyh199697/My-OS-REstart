#include "type.h"
#include "config.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void cleanup(struct proc * proc);


PUBLIC int do_fork(){
	struct proc* p = proc_table;
	int i;
	for(i = 0; i < NR_TASKS + NR_NATIVE_PROCS; i++, p++){
		if(p->p_flags == FREE_SLOT){
			break;
		}
	}
	int child_pid = i;
	assert(p == &proc_table[child_pid]);
	assert(child_pid >= NR_TASKS + NR_NATIVE_PROCS);
	if(i == NR_TASKS + NR_PROCS){
		return -1;
	}
	assert(i < NR_TASKS + NR_PROCS);

	int pid = mm_msg.source;
	u16 child_ldt_sel = p->ldt_sel;
	*p = proc_table[pid];
	p->ldt_sel = child_ldt_sel;
	p->p_parent = pid;
	sprintf(p->p_name, "%s_%d", proc_table[pid].p_name, child_pid);

	struct descriptor * ppd;

	ppd = &proc_table[pid].ldts[INDEX_LDT_C];

	/* base of T-seg, in bytes */
	int caller_T_base  = reassembly(ppd->base_high, 24,
					ppd->base_mid,  16,
					ppd->base_low);
	/* limit of T-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	int caller_T_limit = reassembly(0, 0,
					(ppd->limit_high_attr2 & 0xF), 16,
					ppd->limit_low);
	/* size of T-seg, in bytes */
	int caller_T_size  = ((caller_T_limit + 1) *
			      ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
			       4096 : 1));

	/* Data & Stack segments */
	ppd = &proc_table[pid].ldts[INDEX_LDT_RW];
	/* base of D&S-seg, in bytes */
	int caller_D_S_base  = reassembly(ppd->base_high, 24,
					  ppd->base_mid,  16,
					  ppd->base_low);
	/* limit of D&S-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	int caller_D_S_limit = reassembly((ppd->limit_high_attr2 & 0xF), 16,
					  0, 0,
					  ppd->limit_low);
	/* size of D&S-seg, in bytes */
	int caller_D_S_size  = ((caller_T_limit + 1) *
				((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
				 4096 : 1));

	/* we don't separate T, D & S segments, so we have: */
	assert((caller_T_base  == caller_D_S_base ) &&
	       (caller_T_limit == caller_D_S_limit) &&
	       (caller_T_size  == caller_D_S_size ));
	       
	int child_base = alloc_mem(child_pid, caller_T_size);
	
	printl("{MM} 0x%x <- 0x%x (0x%x bytes)\n",
	       child_base, caller_T_base, caller_T_size);
	phys_copy((void*)child_base, (void*)caller_T_base, caller_T_size);
	
	init_descriptor(&p->ldts[INDEX_LDT_C],
		  child_base,
		  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
	init_descriptor(&p->ldts[INDEX_LDT_RW],
		  child_base,
		  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);
		  
	MESSAGE msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = child_pid;
	send_recv(BOTH, TASK_FS, &msg2fs);
	
	/* child PID will be returned to the parent proc */
	mm_msg.PID = child_pid;
	
	MESSAGE m;
	m.type = SYSCALL_RET;
	m.RETVAL = 0;
	m.PID = 0;
	send_recv(SEND, child_pid, &m);

	return 0;
}

PUBLIC void do_exit(int status){
	int i;
	int pid = mm_msg.source;
	int parent_pid = proc_table[pid].p_parent;
	struct proc* p = &proc_table[pid];
	MESSAGE msg2fs;
	msg2fs.type = EXIT;
	msg2fs.PID = pid;
	send_recv(BOTH, TASK_FS, &msg2fs);
	
	free_mem(pid);
	
	p->exit_status = status;
	if(proc_table[parent_pid].p_flags & WAITING){
		proc_table[parent_pid].p_flags &= ~WAITING;
		cleanup(&proc_table[pid]);
	}else{
		proc_table[pid].p_flags |= HANGING;
	}
	
	for(i = 0; i < NR_TASKS + NR_PROCS; i++){
		if(proc_table[i].p_parent == pid){
			proc_table[i].p_parent = INIT;
			if((proc_table[INIT].p_flags & WAITING)
			 && (proc_table[i].p_flags & HANGING)){
				proc_table[INIT].p_flags = ~WAITING;
				cleanup(&proc_table[i]);
			}
		}
	}
}

PUBLIC void do_wait(){
	int pid = mm_msg.source;
	int i;
	int children = 0;
	struct proc* p_proc = proc_table;
	for(i = 0; i < NR_TASKS + NR_PROCS; i++, p_proc++){
		if(p_proc->p_parent == pid){
			children++;
			if(p_proc->p_flags & HANGING){
				cleanup(p_proc);
				return;
			}
		}
	}
	if(children){
		proc_table[pid].p_flags |= WAITING;
	}else{
		MESSAGE msg;
		msg.type = SYSCALL_RET;
		msg.PID = NO_TASK;
		send_recv(SEND, pid, &msg);
	}
}

PRIVATE void cleanup(struct proc* proc){
	MESSAGE msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(proc);
	msg2parent.STATUS = proc->exit_status;
	send_recv(SEND, proc->p_parent, &msg2parent);
	
	proc->p_flags = FREE_SLOT;//清除之后p_parent不清除，所以init调用wait不会报错
}
