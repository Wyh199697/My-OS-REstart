#ifndef _ORANGES_CONST_H_
#define _ORANGES_CONST_H_

#define PUBLIC
#define PRIVATE static

#define EXTERN extern

#define GDT_SIZE 128
#define IDT_SIZE 256

#define PRIVILEGE_KRNL 0
#define PRIVILEGE_TASK 1
#define PRIVILEGE_USER 3

#define INT_M_CTL	0X20
#define INT_M_CTLMASK 0X21
#define INT_S_CTL	0XA0
#define INT_S_CTLMASK 0xA1

#endif
