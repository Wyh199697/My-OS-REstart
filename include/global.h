#ifndef GLOBAL_VARIABLES_HERE
#undef EXTERN
#define EXTERN
#endif

EXTERN int disp_pos;
EXTERN u8 gdt_ptr[6];
EXTERN DESCRIPTOR gdt[GDT_SIZE];
EXTERN u8 idt_ptr[6];
EXTERN GATE idt[IDT_SIZE];
EXTERN PROCESS proc_table[NR_TASKS+NR_PROCS];
EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready;
EXTERN char task_stack[STACK_SIZE_TOTAL];
EXTERN int k_reenter;
extern TASK task_table[];
extern TASK user_proc_table[];
EXTERN irq_handler irq_table[NR_IRQ];
extern system_call sys_call_table[NR_SYS_CALL]; //删掉试试
EXTERN int ticks;
EXTERN TTY tty_table[NR_CONSOLES];
EXTERN CONSOLE console_table[NR_CONSOLES];
EXTERN	int nr_current_console;
