#define GLOBAL_VARIABLES_HERE //定义了这个宏，在这个文件里就是定义和声明，因为变量前面没有了extern

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"

TASK task_table[NR_TASKS] = {{TestA, STACK_SIZE_TESTA, "TestA"}, 
			     {TestB, STACK_SIZE_TESTB, "TestB"},
			     {TestC, STACK_SIZE_TESTC, "TestC"},
			     {task_tty, STACK_SIZE_TTY, "tty"}, 
			     /*{in_process, STACK_SIZE_PROCESS, "process"}*/};
system_call sys_call_table[NR_SYS_CALL] = {sys_get_ticks};
