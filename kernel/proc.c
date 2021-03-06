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

PRIVATE int  msg_send(struct proc* current, int dest, MESSAGE* m);
PRIVATE int  msg_receive(struct proc* current, int src, MESSAGE* m);
PRIVATE void block(struct proc* p);
PRIVATE void unblock(PROCESS* p);
PRIVATE int deadlock(int src, int dest);

PRIVATE int  msg_send(struct proc* current, int dest, MESSAGE* m){//current主动发送消息，dest被动接受消息，进入这里的时候允许硬件中断，那么时钟中断就会发生，然后进行进程调度(大误)，实际上ring0的时候会相应中断，但是不会处理中断，所以不会发生死锁
	PROCESS* sender = current;
	PROCESS* p_dest = proc_table + dest;
	assert(proc2pid(sender) != dest); //garantee the sender is not itself
	if(deadlock(proc2pid(sender),dest)){
		panic(">>DEADLOCK<< %s->%s", sender->p_name, p_dest->p_name);
	}
	if((p_dest->p_flags & RECEIVING) && 
		(p_dest->p_recvfrom == proc2pid(sender) ||
		 p_dest->p_recvfrom == ANY)){
		assert(p_dest->p_msg);
		assert(m);
		phys_copy(va2la(dest, p_dest->p_msg),
				  va2la(proc2pid(sender), m),
				  sizeof(MESSAGE));
		p_dest->p_msg = 0;
		p_dest->p_flags &= ~RECEIVING;
		p_dest->p_recvfrom = NO_TASK;
		unblock(p_dest);

		assert(p_dest->p_flags == 0);
		assert(p_dest->p_msg == 0);
		assert(p_dest->p_recvfrom == NO_TASK);
		assert(p_dest->p_sendto == NO_TASK);
		assert(sender->p_flags == 0);
		assert(sender->p_msg == 0);
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == NO_TASK);
	}else{
		sender->p_flags |= SENDING;
		assert(sender->p_flags == SENDING);
		sender->p_sendto = dest;
		sender->p_msg = m;

		PROCESS* p;
		if(p_dest->q_sending){
			p = p_dest->q_sending;
			while(p->next_sending){
				p = p->next_sending;
			}
			p->next_sending = sender;
		}else{
			p_dest->q_sending = sender;
		}
		sender->next_sending = 0;

		block(sender);

		assert(sender->p_flags == SENDING);
		assert(sender->p_msg != 0);
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == dest);
	}
	return 0;
}

PRIVATE int  msg_receive(struct proc* current, int src, MESSAGE* m){
	PROCESS* p_who_wanna_recv = current;
	PROCESS* p_from = 0;
	PROCESS* prev = 0;
	int copyok = 0;

	assert(proc2pid(p_who_wanna_recv) != src);

	if((p_who_wanna_recv->has_int_msg) && ((src == ANY) || (src == INTERRUPT))){
		MESSAGE msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;

		assert(m);

		phys_copy(va2la(proc2pid(p_who_wanna_recv), m), &msg, 
				sizeof(MESSAGE));

		p_who_wanna_recv->has_int_msg = 0;

		assert(p_who_wanna_recv->p_flags == 0);
		assert(p_who_wanna_recv->p_msg == 0);
		assert(p_who_wanna_recv->p_sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);
		return 0;
	}
	if(src == ANY){
		if(p_who_wanna_recv->q_sending){
			p_from = p_who_wanna_recv->q_sending;
			copyok = 1;

			assert(p_who_wanna_recv->p_flags == 0);
			assert(p_who_wanna_recv->p_msg == 0);
			assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			assert(p_who_wanna_recv->p_sendto == NO_TASK);
			assert(p_who_wanna_recv->q_sending != 0);
			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg != 0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
		}
	}else if(src >= 0 && src < NR_TASKS + NR_PROCS){
		p_from = &proc_table[src];
		if((p_from->p_flags & SENDING) && (p_from->p_sendto == proc2pid(p_who_wanna_recv))){
			copyok = 1;
			PROCESS* p = p_who_wanna_recv->q_sending;
			assert(p);
			while(p){
				assert(p_from->p_flags & SENDING);
				if(proc2pid(p) == src){
					break;
				}
				prev = p;
				p = p->next_sending;
			}

			assert(p_who_wanna_recv->p_flags == 0);
			assert(p_who_wanna_recv->p_msg == 0);
			assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			assert(p_who_wanna_recv->p_sendto == NO_TASK);
			assert(p_who_wanna_recv->q_sending != 0);
			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg != 0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
		}
	}
	if(copyok){
		if(p_from == p_who_wanna_recv->q_sending){
			assert(prev == 0);
			p_who_wanna_recv->q_sending = p_from->next_sending;
			p_from->next_sending = 0; 
		}else{
			assert(prev);
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		assert(m);
		assert(p_from->p_msg);

		phys_copy(va2la(proc2pid(p_who_wanna_recv), m),
				 va2la(proc2pid(p_from), p_from->p_msg), sizeof(MESSAGE));
		p_from->p_msg = 0;
		p_from->p_msg = 0;
		p_from->p_sendto = NO_TASK;
		p_from->p_flags &= ~SENDING;

		unblock(p_from);
	}else{
		p_who_wanna_recv->p_flags |= RECEIVING;
		p_who_wanna_recv->p_msg = m;
		p_who_wanna_recv->p_recvfrom = src;
		block(p_who_wanna_recv);

		assert(p_who_wanna_recv->p_flags = RECEIVING);
		assert(p_who_wanna_recv->p_msg != 0);
		assert(p_who_wanna_recv->p_recvfrom != NO_TASK);
		assert(p_who_wanna_recv->p_sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);
	}
	return 0;
}

PRIVATE void block(PROCESS* p){
	assert(p->p_flags);
	schedule();
}

PRIVATE void unblock(PROCESS* p){
	assert(p->p_flags == 0);
}

PRIVATE int deadlock(int src, int dest){ //wtf?
	PROCESS* p = proc_table + dest;
	while(1){
		if(p->p_flags & SENDING){ //if the state of proc is sending
			if(p->p_sendto == src){//if the two processes are not the objects of detection, the routine will loop forever?A->B->C->B->C
				p = proc_table + dest;
				printl("=_=%s", p->p_name);
				do{
					assert(p->p_msg);
					p = proc_table + p->p_sendto;
					printf("->%s", p->p_name);
				}while(p != proc_table + src);
				printf("=_=");
				return 1;
			}
			p = proc_table + p->p_sendto;
		}else{
			break;
		}
	}
	return 0;
}

PUBLIC int ldt_seg_linear(struct proc* p, int idx)
{
	struct descriptor * d = (struct descriptor*)&p->ldts[idx];

	return d->base_high << 24 | d->base_mid << 16 | d->base_low; // return ldt of process segment linear address
}

PUBLIC void* va2la(int pid, void* va)
{
	struct proc* p = &proc_table[pid];

	u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va;

	if (pid < NR_TASKS + NR_NATIVE_PROCS) {
		assert(la == (u32)va); //the base address of data ldt(seg_base) should be 0,so the la == va.
	}

	return (void*)la; //return virtual address
}

PUBLIC void reset_msg(MESSAGE*  p){
	memset(p, 0, sizeof(MESSAGE));
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
/*PUBLIC int sys_get_ticks()
{
	//disp_str("+");
	return ticks;
}*/

PUBLIC void schedule(){
	PROCESS* p;
	int greatest_ticks = 0;
	while(!greatest_ticks){
		for(p = &FIRST_PROC; p <= &LAST_PROC; ++p){
			if(p->p_flags == 0){
				if(p->ticks > greatest_ticks){//选出ticks最多而且flags为0的那个进程
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
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
				if(p->p_flags == 0){
					p->ticks = p->priority;//如果所有进程ticks都为0，那么重置ticks
				}
			}
		}
	}
}

PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m,  struct proc* p){
	assert(k_reenter == 0);/*make sure we are not in ring0*/
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) || 
			src_dest == ANY || 
			src_dest == INTERRUPT);

	int ret = 0;
	int caller = proc2pid(p);
	MESSAGE* mla = (MESSAGE*)va2la(caller, m);
	mla->source = caller;
	assert(mla->source != src_dest);
	if(function == SEND){
		ret = msg_send(p, src_dest, m);
		if(ret != 0){
			return ret;
		}
	}else if(function == RECEIVE){
			ret = msg_receive(p, src_dest, m);
			if(ret != 0){
				return ret;
			}
	}else{
		panic("{sys_sendrec} invalid function: "
					"%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
		}
	return 0;
}

PUBLIC void inform_int(int task_nr){
	struct proc* p = proc_table + task_nr;

	if((p->p_flags & RECEIVING) && ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))){
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING;
		p->p_recvfrom = NO_TASK;
		assert(p->p_flags == 0);
		unblock(p);//这句才是重点，取消进程阻塞

		assert(p->p_flags == 0);
		assert(p->p_msg == 0);
		assert(p->p_recvfrom == NO_TASK);
		assert(p->p_sendto == NO_TASK);
	}else{
		p->has_int_msg = 1;//相当于发送中断的机制在SENDING
	}
}

PUBLIC void dump_msg(const char * title, MESSAGE* m)
{
	int packed = 0;
	printl("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)m,
	       packed ? "" : "\n        ",
	       proc_table[m->source].p_name,
	       m->source,
	       packed ? " " : "\n        ",
	       m->type,
	       packed ? " " : "\n        ",
	       m->u.m3.m3i1,
	       m->u.m3.m3i2,
	       m->u.m3.m3i3,
	       m->u.m3.m3i4,
	       (int)m->u.m3.m3p1,
	       (int)m->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}

