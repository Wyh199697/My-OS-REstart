#include "type.h"
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
#include "config.h"
#include "stdio.h"

#include "elf.h"


PUBLIC void get_boot_params(struct boot_params * pbp)
{
	/**
	 * Boot params should have been saved at BOOT_PARAM_ADDR.
	 */
	int * p = (int*)BOOT_PARAM_ADDR;
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	pbp->mem_size = p[BI_MEM_SIZE];
	pbp->kernel_file = (unsigned char *)(p[BI_KERNEL_FILE]);

	/**
	 * the kernel file should be a ELF executable,
	 * check it's magic number
	 */
	assert(memcmp(pbp->kernel_file, ELFMAG, SELFMAG) == 0);
}

PUBLIC int get_kernel_map(unsigned int * b, unsigned int * l)
{
	struct boot_params bp;
	get_boot_params(&bp);

	Elf32_Ehdr* elf_header = (Elf32_Ehdr*)(bp.kernel_file);

	/* the kernel file should be in ELF format */
	if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0)
		return -1;

	*b = ~0;
	unsigned int t = 0;
	int i;
	for (i = 0; i < elf_header->e_shnum; i++) {
		Elf32_Shdr* section_header =
			(Elf32_Shdr*)(bp.kernel_file +
				      elf_header->e_shoff +
				      i * elf_header->e_shentsize);

		if (section_header->sh_flags & SHF_ALLOC) {
			int bottom = section_header->sh_addr;
			int top = section_header->sh_addr +
				section_header->sh_size;

			if (*b > bottom)
				*b = bottom;
			if (t < top)
				t = top;
		}
	}
	assert(*b < t);
	*l = t - *b - 1;

	return 0;
}

PUBLIC char* itoa(char * str, int num){
	char * p = str;
	char ch;
	int i;
	int flag = 0;
	*p++ = '0';
	*p++ = 'x';
	if(num == 0){
		*p++ = '0';
	}else{
		for(i = 28; i >= 0; i -= 4){
			ch = (num >> i) & 0xf;
			if(flag || (ch > 0)){
				flag = 1;
				ch += '0';
				if(ch > '9'){
					ch += 7;
				}
				*p++ = ch;
			}
		}
	}
	*p = 0;
	return str;
}
//============

PUBLIC void disp_int(int input){
	char ouput[16];
	disp_str(itoa(ouput, input));
}

/*PUBLIC void delay(int time){
	int i, j, k;
	for(i = 0; i < time; ++i){
		for(j = 0; j < 10; ++j){
			for(k = 0; k < 10000; ++k){
			}
		}
	}
}*/
