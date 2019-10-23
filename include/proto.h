//#ifndef _ORANGES,为什么不用写ifdef，因为函数声明可以多次，但是宏定义不能多次

PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char * info);
PUBLIC void init_prot();
PUBLIC void init_8259A();
PUBLIC void disp_color_str(char * info, int num);
PUBLIC void disp_int(int input);
//PUBLIC void delay(int time);
PUBLIC void milli_dealy(int milli_sec);
PUBLIC u32 seg2phys(u16 seg);
PUBLIC void TestA();
PUBLIC void TestB();
PUBLIC void TestC();
PUBLIC void restart();
PUBLIC char* strcpy(char*src, char*dst);
PUBLIC void disable_irq(int irq);
PUBLIC void enable_irq(int irq);
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);
PUBLIC void clock_handler(int irq);
PUBLIC int get_ticks();
PUBLIC void sys_call();
PUBLIC int sys_get_ticks();
