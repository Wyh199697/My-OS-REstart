#ifndef _ORANGES_PROC_H_
#define _ORANGES_PROC_H_


typedef struct stackframe {
	u32	gs;		/* \                                    */
	u32	fs;		/* |                                    */
	u32	es;		/* |                                    */
	u32	ds;		/* |                                    */
	u32	edi;		/* |                                    */
	u32	esi;		/* | pushed by save()                   */
	u32	ebp;		/* |                                    */
	u32	kernel_esp;	/* <- 'popad' will ignore it            */
	u32	ebx;		/* |                                    */
	u32	edx;		/* |                                    */
	u32	ecx;		/* |                                    */
	u32	eax;		/* /                                    */
	u32	retaddr;	/* return addr for kernel.asm::save()   */
	u32	eip;		/* \                                    */
	u32	cs;		/* |                                    */
	u32	eflags;		/* | pushed by CPU during interrupt     */
	u32	esp;		/* |                                    */
	u32	ss;		/* /                                    */
}STACK_FRAME;


typedef struct proc {
	STACK_FRAME regs;          /* process registers saved in stack frame */
	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */
	int ticks;
	int priority;
	u32 pid;                   /* process id passed in from MM */
	char p_name[16];           /* name of the process */
	int p_flags;
	MESSAGE* p_msg;
	int p_recvfrom;
	int p_sendto;
	int has_int_msg;
	struct proc* q_sending;
	struct proc* next_sending;
	int nr_tty;
	
	int p_parent; /**< pid of parent process */

	int exit_status; /**< for parent */
	struct file_desc* filp[NR_FILES];
}PROCESS;

typedef struct task{
	task_f initial_eip;
	int stacksize;
	char name[32];
	
}TASK;

/**
 * All forked proc will use memory above PROCS_BASE.
 *
 * @attention make sure PROCS_BASE is higher than any buffers, such as
 *            fsbuf, mmbuf, etc
 * @see global.c
 * @see global.h
 */
#define	PROCS_BASE		0xA00000 /* 10 MB */
#define	PROC_IMAGE_SIZE_DEFAULT	0x100000 /*  1 MB */
#define	PROC_ORIGIN_STACK	0x400    /*  1 KB */

/* Number of tasks */
#define NR_TASKS	5
#define NR_PROCS	32
#define NR_NATIVE_PROCS	4
#define FIRST_PROC	proc_table[0]
#define LAST_PROC	proc_table[NR_TASKS + NR_PROCS - 1]

#define proc2pid(x) (x - proc_table)

/* stacks of tasks */
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TESTB	0x8000
#define STACK_SIZE_TESTC	0x8000
#define STACK_SIZE_TTY		0x8000
#define STACK_SIZE_SYS		0x8000
#define STACK_SIZE_HD		0x8000
#define STACK_SIZE_FS		0x8000
#define STACK_SIZE_INIT		0x8000
#define STACK_SIZE_MM		0x8000

#define STACK_SIZE_TOTAL	STACK_SIZE_TESTA + STACK_SIZE_TESTB + STACK_SIZE_TESTC+STACK_SIZE_TTY + STACK_SIZE_SYS + STACK_SIZE_HD + STACK_SIZE_FS + STACK_SIZE_INIT + STACK_SIZE_MM


#endif
