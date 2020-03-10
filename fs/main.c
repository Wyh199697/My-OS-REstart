#include "type.h"
#include "stdio.h"
#include "config.h"
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

PRIVATE void init_fs();
PRIVATE void mkfs();
PRIVATE void read_super_block(int dev);

//系统开始运行task_fs

PUBLIC void task_fs(){
	printl("Task FS begins.\n");
	init_fs();
	while(1){
		send_recv(RECEIVE, ANY, &fs_msg);
		int src = fs_msg.source;
		pcaller = &proc_table[src];
		int msgtype = fs_msg.type;
		switch(msgtype){
			case OPEN:
				fs_msg.FD = do_open();
				break;
			case CLOSE:
				fs_msg.RETVAL = do_close();
				break;
			case READ:	
			case WRITE:
				fs_msg.CNT = do_rdwt();
				break;
			case UNLINK:
				fs_msg.RETVAL = do_unlink();
				break;
			case RESUME_PROC:
				src = fs_msg.PROC_NR;
				break;
			default:
				dump_msg("FS::unknown message:", &fs_msg);
				assert(0);
				break;
		}
		
	/*#ifdef ENABLE_DISK_LOG
		char * msg_name[128];
		msg_name[OPEN]   = "OPEN";
		msg_name[CLOSE]  = "CLOSE";
		msg_name[READ]   = "READ";
		msg_name[WRITE]  = "WRITE";
		msg_name[LSEEK]  = "LSEEK";
		msg_name[UNLINK] = "UNLINK";
		/* msg_name[FORK]   = "FORK"; */
		/* msg_name[EXIT]   = "EXIT"; */
		/* msg_name[STAT]   = "STAT"; */

		/*switch (msgtype) {
		case OPEN:
		case CLOSE:
		case READ:
		case WRITE:
		/* case FORK: */
			//dump_fd_graph("%s just finished.", msg_name[msgtype]);
			//panic("");
		/* case LSEEK: */
		/* case UNLINK: */
		/* case EXIT: */
		/* case STAT: */
			//break;
		/* case RESUME_PROC: */
		/*case DISK_LOG:
			break;
		default:
			assert(0);
		}
	#endif*/
		if (fs_msg.type != SUSPEND_PROC) {
			fs_msg.type = SYSCALL_RET;
			send_recv(SEND, src, &fs_msg);
		}
	}

}

PRIVATE void init_fs(){//给task_hd发送消息
	//printf("Task FS begin.\n");
	int i;
	for(i = 0; i < NR_FILE_DESC; i++){
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
	}
	for(i = 0; i < NR_INODE; i++){
		memset(&inode_table[i], 0, sizeof(struct inode));
	}
	struct super_block* sb = super_block;
	for(; sb < &super_block[NR_SUPER_BLOCK]; sb++){
		sb->sb_dev = NO_DEV;
	}
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV); //msg.DEVICE可以使主设备号也可以是次设备号
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	mkfs();

	read_super_block(ROOT_DEV);
	sb = get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
	//spin("FS");
}

PRIVATE void mkfs(){
	MESSAGE driver_msg;
	int i, j;
	//每个扇区的bit数
	int bits_per_sect = SECTOR_SIZE * 8;

	struct part_info geo;
	driver_msg.type = DEV_IOCTL;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	driver_msg.REQUEST = DIOCTL_GET_GEO;
	driver_msg.BUF = &geo;
	driver_msg.PROC_NR = TASK_FS;
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg); //得到ROOT_DEV的起始扇区和扇区数
	printl("dev size: 0x%x sectors\n", geo.size);
	
	//super_block的值
	struct super_block sb;
	sb.magic = MAGIC_V1;
	sb.nr_inodes = bits_per_sect;
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE; //4096*32/512
	sb.nr_sects = geo.size;
	sb.nr_imap_sects = 1;
	sb.nr_smap_sects = sb.nr_sects / bits_per_sect + 1; //sector-map数量
	sb.n_1st_sect = 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
	sb.root_inode = ROOT_INODE;
	sb.inode_size = INODE_SIZE;
	struct inode x;
	sb.dir_ent_inode_off = (int)&x.i_size - (int)&x;
	sb.inode_start_off = (int)&x.i_start_sect - (int)&x;
	sb.dir_ent_size = DIR_ENTRY_SIZE;
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
	sb.dir_ent_fname_off = (int)&de.name - (int)&de;

	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);

	WR_SECT(ROOT_DEV, 1);

	printl("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
	       "        inodes:0x%x00, 1st_sector:0x%x00\n", 
	       geo.base * 2,
	       (geo.base + 1) * 2,
	       (geo.base + 1 + 1) * 2,
	       (geo.base + 1 + 1 + sb.nr_imap_sects) * 2,
	       (geo.base + 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects) * 2,
	       (geo.base + sb.n_1st_sect) * 2);
	       
	//初始化inode map
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i = 0; i < (NR_CONSOLES + 2); i++){
		fsbuf[0] |= 1 << i;
	}

	assert(fsbuf[0] == 0X1F);
	WR_SECT(ROOT_DEV, 2); //写入inode map位图

	//初始化sector map
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	for(i = 0; i < nr_sects / 8; i++){ //256
		fsbuf[i] = 0xFF;
	}
	for(j = 0; j < nr_sects % 8; j++){
		fsbuf[i] |= (1 << j);
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects); //写入sector map

	//初始化sector map，根目录占用2048个扇区
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i = 1; i < sb.nr_smap_sects; i++){
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i);
	}

	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4;
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	for(i = 0; i < NR_CONSOLES; i++){
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects);

	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry* pde = (struct dir_entry*)fsbuf;
	pde->inode_nr = 1;
	strcpy(pde->name, ".");

	for(i = 0; i < NR_CONSOLES; i++){
		pde++;
		pde->inode_nr = i + 2;
		sprintf(pde->name, "dev_tty%d", i);
	}
	WR_SECT(ROOT_DEV, sb.n_1st_sect);
}

PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf){
	MESSAGE driver_msg;
	driver_msg.type = io_type;
	driver_msg.DEVICE = MINOR(dev);
	driver_msg.POSITION = pos;
	driver_msg.BUF = buf;
	driver_msg.CNT = bytes;
	driver_msg.PROC_NR = proc_nr;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
}

//根据inode_nr，返回inode指针，指针指向inode_table一个成员。现将inode_array，读取到内存中，然后根据num选取inode赋值给inode_table一个成员。总共可以打开64个文件。
PUBLIC struct inode* get_inode(int dev, int num){
	if(num == 0){
		return 0;
	}
	struct inode* p;
	struct inode* q = 0;
	for(p = &inode_table[0]; p < &inode_table[NR_INODE]; p++){
		if(p->i_cnt){//如果有进程占用这个inode
			if((p->i_dev == dev) && (p->i_num == num)){
				p->i_cnt++;
				return p;
			}
		}else{
			if(!q){
				q = p;
			}
		}
	}
	if(!q){
		panic("the inode table is full");
	}
	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;
	
	struct super_block* sb = get_super_block(dev);//得到次设备号为dev的super_block指针
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(dev, blk_nr);
	struct inode* pinode = (struct inode*)((u8*)fsbuf + (num - 1) % (SECTOR_SIZE / INODE_SIZE) * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}
//将指定扇区读入内存，该先指定inode，在写入扇区
PUBLIC void sync_inode(struct inode* p){
	struct inode* pinode;
	struct super_block* sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(p->i_dev, blk_nr);
	pinode = (struct inode*)((u8*)fsbuf + (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE))*INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr);
}

//从super_block_table找到设备号相同的超级快并返回指针
PUBLIC struct super_block* get_super_block(int dev){
	struct super_block* sb = super_block;
	for(; sb < &super_block[NR_SUPER_BLOCK]; sb++){
		if(sb->sb_dev == dev){
			return sb;
		}
	}
	panic("super block of device %d not found.\n", dev);
	return 0;
}

//将指定设备号的超级快读入缓存
PRIVATE void read_super_block(int dev){
	int i;
	MESSAGE driver_msg;

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= fsbuf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= TASK_FS;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	
	for(i = 0; i < NR_SUPER_BLOCK; i++){
		if(super_block[i].sb_dev == NO_DEV){
			break;
		}
	}
	if(i == NR_SUPER_BLOCK){
		panic("super_block slots used up");
	}
	assert(i == 0);
	struct super_block* psb = (struct super_block*)fsbuf;
	super_block[i] = *psb;
	super_block[i].sb_dev = dev;
}

PUBLIC void put_inode(struct inode* pinode){
	assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}
