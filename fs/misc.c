#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"
#include "hd.h"
#include "fs.h"

PUBLIC int do_stat()
{
	char pathname[MAX_PATH]; /* parameter from the caller */
	char filename[MAX_PATH]; /* directory has been stipped */

	/* get parameters from the message */
	int name_len = fs_msg.NAME_LEN;	/* length of filename */
	int src = fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_FS, pathname),    /* to   */
		  (void*)va2la(src, fs_msg.PATHNAME), /* from */
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		printl("{FS} FS::do_stat():: search_file() returns "
		       "invalid inode: %s\n", pathname);
		return -1;
	}

	struct inode * pin = 0;

	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0) {
		/* theoretically never fail here
		 * (it would have failed earlier when
		 *  search_file() was called)
		 */
		assert(0);
	}
	pin = get_inode(dir_inode->i_dev, inode_nr);

	struct stat s;		/* the thing requested */
	s.st_dev = pin->i_dev;
	s.st_ino = pin->i_num;
	s.st_mode= pin->i_mode;
	s.st_rdev= is_special(pin->i_mode) ? pin->i_start_sect : NO_DEV;
	s.st_size= pin->i_size;

	put_inode(pin);

	phys_copy((void*)va2la(src, fs_msg.BUF), /* to   */
		  (void*)va2la(TASK_FS, &s),	 /* from */
		  sizeof(struct stat));

	return 0;
}

PUBLIC int search_file(char* path){
	int i, j;
	char filename[MAX_PATH];//文件名
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode* dir_inode;
	if(strip_path(filename, path, &dir_inode) != 0){
		return 0;
	}
	if(filename[0] == 0){
		return dir_inode->i_num;
	}
	int dir_blk0_nr = dir_inode->i_start_sect;//文件起始扇区
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;//文件占用扇区数
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE;//文件大小除以文件描述符的大小
	int m = 0;
	struct dir_entry* pde;
	for(i = 0; i < nr_dir_blks; i++){
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry*)fsbuf;
		for(j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++){
			if(memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0){
				return pde->inode_nr;
			}
			if(++m > nr_dir_entries){
				break;
			}
		}
		if(m > nr_dir_entries){
			break;
		}
	}
	return 0;
}

PUBLIC int strip_path(char * filename, const char * path, struct inode** ppinode){
	const char* s = path;
	char* t = filename;
	if(s == 0){
		return -1;//路径为空
	}
	if(*s == '/'){
		s++;
	}
	while(*s){
		if(*s == '/'){
			return -1;//路径不合法
		}
		*t++ = *s++;
		if(t - filename >= MAX_FILENAME_LEN){
			break;
		}
	}
	*t = 0;
	*ppinode = root_inode; //??
	return 0;
}
