;SELECTOR_KERNEL_CS equ 8
%include "sconst.inc"

extern cstart
extern exception_handler
extern spurious_irq
extern kernel_main
extern disp_str
extern delay
extern clock_handler
extern sys_call_table

extern gdt_ptr
extern idt_ptr
extern disp_pos
extern p_proc_ready
extern tss
extern k_reenter
extern irq_table

[SECTION .data]
clock_int_msg	db '^',0

[SECTION .bss]
StackSpace	resb	2 * 1024
StackTop:

[section .text]

global _start
global restart

global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
global  hwint00
global  hwint01
global  hwint02
global  hwint03
global  hwint04
global  hwint05
global  hwint06
global  hwint07
global  hwint08
global  hwint09
global  hwint10
global  hwint11
global  hwint12
global  hwint13
global  hwint14
global  hwint15
global sys_call

_start:
	mov esp, StackTop
	sgdt [gdt_ptr]	;将原GDTPTR的值保存到gdt_ptr
	mov dword [disp_pos],0
	call cstart
	lgdt [gdt_ptr]
	lidt [idt_ptr]

	jmp SELECTOR_KERNEL_CS:csinit

csinit:
	;ud2
	;jmp 0x40:0
	;sti
	;hlt
	xor eax,eax
	mov ax,SELECTOR_TSS
	ltr ax
	jmp SELECTOR_KERNEL_CS:kernel_main
	

; ---------------------------------
%macro  hwint_master	1
	call	save
	in	al, INT_M_CTLMASK	; `.
	or	al, (1 << %1)		;  | 屏蔽当前中断
	out	INT_M_CTLMASK, al	; /
	mov	al, EOI			; `. 置EOI位
	out	INT_M_CTL, al		; /
	sti	; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push	%1			; `.
	call	[irq_table + 4 * %1]	;  | 中断处理程序
	pop	ecx			; /
	cli
	in	al, INT_M_CTLMASK	; `.
	and	al, ~(1 << %1)		;  | 恢复接受当前中断
	out	INT_M_CTLMASK, al	; /
	ret
%endmacro
; ---------------------------------

ALIGN   16
hwint00: ; Interrupt routine for irq 0 (the clock).
	hwint_master	0
;	sub esp,4
;	pushad
;	push ds
;	push es
;	push fs
;	push gs
;	mov dx,ss
;	mov ds,dx
;	mov es,dx

;	call save
	
;    inc	byte [gs:0]	; 改变屏幕第 0 行, 第 0 列的字符
;	in al, INT_M_CTLMASK
;	or al,1
;	out INT_M_CTLMASK,al
;
;	mov	al, EOI		; `. reenable
;	out	INT_M_CTL, al	; / master 8259

;	inc dword [k_reenter]
;	cmp dword [k_reenter],0
;	jne .1

;	mov esp,StackTop		;没有重入中断，转到内核栈

;	push restart		
;	jmp .2

;.1:							;如果重入中断，那么不用再设置内核栈
;	push restart_reenter
;.2:	
;	sti
;
;	push 0
;	call clock_handler
;	add esp,4

	;push 1000		;因为中断重入与否都会执行代码，所以如果延迟时间过长哪又会发生中断重入，于是函数就不能出栈，最终发生栈溢出
	;call delay
	;add esp,4
	;push clock_int_msg
	;call disp_str
	;add esp,4		;msg是文件内的偏移加上程序的入口地址=线性地址(物理地址)
	;push 10
	;call delay
	;add esp,4

;	cli

;	in al,INT_M_CTLMASK
;	and al,0xfe
;	out INT_M_CTLMASK,al
;
;	ret							;返回到上面push的地址处

;.restart_v2:

	;mov esp,[p_proc_ready]		;p_proc_ready在clock_handler函数中改变了
	;lldt [esp+P_LDT_SEL]
	;lea eax,[esp+P_STACKTOP]
	;mov dword [tss+TSS3_S_SP0],eax ;主要是因为多进程要不断改变sp0的值

;.restart_reenter_v2:
	;dec dword[k_reenter]
	;pop gs
	;pop fs
	;pop es
	;pop ds
	;popad
	;add esp,4
	;iretd

ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7
	
; ---------------------------------
%macro  hwint_slave     1
        push    %1
        call    spurious_irq
        add     esp, 4
        hlt
%endmacro
; ---------------------------------

ALIGN   16
hwint08:                ; Interrupt routine for irq 8 (realtime clock).
        hwint_slave     8

ALIGN   16
hwint09:                ; Interrupt routine for irq 9 (irq 2 redirected)
        hwint_slave     9

ALIGN   16
hwint10:                ; Interrupt routine for irq 10
        hwint_slave     10

ALIGN   16
hwint11:                ; Interrupt routine for irq 11
        hwint_slave     11

ALIGN   16
hwint12:                ; Interrupt routine for irq 12
        hwint_slave     12

ALIGN   16
hwint13:                ; Interrupt routine for irq 13 (FPU exception)
        hwint_slave     13

ALIGN   16
hwint14:                ; Interrupt routine for irq 14 (AT winchester)
        hwint_slave     14

ALIGN   16
hwint15:                ; Interrupt routine for irq 15
        hwint_slave     15

	
divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
double_fault:
	push	8		; vector_no	= 8
	jmp	exception
copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
inval_tss:
	push	10		; vector_no	= A
	jmp	exception
segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
stack_exception:
	push	12		; vector_no	= C
	jmp	exception
general_protection:
	push	13		; vector_no	= D
	jmp	exception
page_fault:
	push	14		; vector_no	= E
	jmp	exception
copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt

save:
	pushad
	push ds
	push es
	push fs
	push gs

	mov esi,edx
	mov dx,ss
	mov ds,dx
	mov es,dx
	mov edx,esi

	mov esi,esp

	inc dword[k_reenter]
	cmp dword[k_reenter],0
	jne .1

	mov esp,StackTop
	push restart
	jmp [esi+RETADR - P_STACKBASE]
.1:
	push restart_reenter
	jmp [esi+RETADR - P_STACKBASE]

restart:
	mov esp, [p_proc_ready]
	lldt [esp + P_LDT_SEL]
	lea eax,[esp + P_STACKTOP]
	mov dword [tss + TSS3_S_SP0],eax
restart_reenter:
	dec dword [k_reenter]
	pop gs
	pop fs
	pop es
	pop ds
	popad

	add esp,4

	iretd

sys_call:
	call save
	push dword [p_proc_ready]
	sti			;允许硬件中断
	push edx ;i
	push ecx ;buf
	push ebx 
	call [sys_call_table+4*eax]
	add esp, 4 * 4
	mov [esi+EAXREG-P_STACKBASE],eax	;保存eax的值
	cli
	ret			;return之后上面的代码会执行iretd
