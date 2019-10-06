//#ifndef _ORANGES,为什么不用写ifdef，因为函数声明可以多次，但是宏定义不能多次

PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char * info);
PUBLIC void init_prot();
PUBLIC void init_8259A();
PUBLIC void disp_color_str(char * info, int num);
PUBLIC void disp_int(int input);
