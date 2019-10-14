#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "proc.h"
#include "global.h"


PUBLIC void kernel_main(){
	disp_str("-----\"kernel_main\" begins-----\n");
	while(1);
}

void TestA(){
	int i = 0;
	while(1){
		disp_str("A");
		disp_int(i++);
		disp_str(".");
		delay(1);
	}
}
