#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proto.h"
#include "proc.h"
#include "global.h"


//PUBLIC void* memcpy(void* pDst, void* pSrc, int iSize);

//PUBLIC void disp_str(char * pszInfo);

//PUBLIC u8	gdt_ptr[6];
//PUBLIC DESCRIPTOR	gdt[GDT_SIZE];

PUBLIC void cstart(){
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "-----\"cstart\" begins-----\n");
	memcpy(&gdt, ((void*)(*((u32*)&gdt_ptr[2]))),*((u16*)&gdt_ptr[0])+1);
	////////////

	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);//
	u32* p_gdt_base = (u32*)(&gdt_ptr[2]);//
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;//
	*p_gdt_base  = (u32)&gdt;

	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
	*p_idt_base = (u32)(&idt);

	init_prot();

	disp_str("-----\"cstart\" ends-----\n");
	
	/*u16 limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	u32 base = (u32)(&gdt);
	gdt_ptr[0] = *(u8*)(&limit);
	gdt_ptr[1] = *((u8*)(&limit) + 1);
	gdt_ptr[2] = *(u8*)(&base);
	gdt_ptr[3] = *((u8*)(&base)+1);
	gdt_ptr[4] = *((u8*)(&base)+2);
	gdt_ptr[5] = *((u8*)(&base)+3);*/
}
