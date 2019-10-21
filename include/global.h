#ifndef GLOBAL_VARIABLES_HERE
#undef EXTERN
#define EXTERN
#endif

EXTERN int disp_pos;
EXTERN u8 gdt_ptr[6];
EXTERN DESCRIPTOR gdt[GDT_SIZE];
EXTERN u8 idt_ptr[6];
EXTERN GATE idt[IDT_SIZE];
EXTERN PROCESS proc_table[NR_TASKS];
EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready;
EXTERN char task_stack[STACK_SIZE_TOTAL];
EXTERN int k_reenter;
extern TASK task_table[];
