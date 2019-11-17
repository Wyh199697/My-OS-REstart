#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"


PUBLIC void kernel_main(){
	disp_str("-----\"kernel_main\" begins-----\n");
	PROCESS *p_proc = proc_table;
	TASK *p_task = task_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	
	//proc_table[0].ticks = proc_table[0].priority = 150;
	//proc_table[1].ticks = proc_table[1].priority = 50;
	//proc_table[2].ticks = proc_table[2].priority = 30;
	proc_table[3].ticks = proc_table[3].priority = 300;
	//proc_table[4].ticks = proc_table[4].priority = 300;

	for(int i = 0; i < NR_TASKS; ++i){
		strcpy(p_proc->p_name, p_task->name);
		p_proc->pid = i;
		p_proc->ldt_sel = selector_ldt; //进程a的ldt的gdt选择子
		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS>>3], sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS>>3], sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;

		p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK; //0是第一个段选择子，将rpl和ti清空之后初始化为ldt和rpl=1
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;
		p_proc->regs.eip= (u32)p_task->initial_eip;
		p_proc->regs.esp= (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202;	// IF=1, IOPL=1, bit 2 is always 1.
		

		p_proc++;
		p_task++;
		p_task_stack -= p_task->stacksize;
		selector_ldt += 1 << 3;
	}

	p_proc_ready	= proc_table;
	k_reenter = 0;
	ticks = 0;

	init_clock();
	init_keyboard();
	
	/*disp_pos = 0;
	for (int i = 0; i < 80*25; i++) {
		disp_str(" ");
	}
	disp_pos = 0;*/
	restart();
	

	while(1);
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1) {
		//disp_color_str("A.", BRIGHT | MAKE_COLOR(BLACK, RED));
		//disp_int(get_ticks());
		milli_delay(200);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while(1){
		//disp_color_str("B.", BRIGHT | MAKE_COLOR(BLACK, RED));
		//disp_int(get_ticks());
		milli_delay(200);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while(1){
		//disp_color_str("C.", BRIGHT | MAKE_COLOR(BLACK, RED));
		//disp_int(get_ticks());
		milli_delay(200);
	}
}
