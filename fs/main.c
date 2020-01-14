#include "type.h"
//#include "config.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

#include "hd.h"

//系统开始运行task_fs，给task_hd发送消息
PUBLIC void task_fs(){
	printf("Task FS begin.\n");
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	send_recv(BOTH, TASK_HD, &driver_msg);

	spin("FS");
}
