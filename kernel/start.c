#include "type.h"
#include "const.h"
#include "protect.h"

PUBLIC void* memcpy(void* pDst, void* pSrc, int iSize);

PUBLIC void disp_str(char * pszInfo);

PUBLIC u8	gdt_ptr[6];
PUBLIC DESCRIPTOR	gdt[GDT_SIZE];

PUBLIC void cstart(){
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "-----\"cstart\" begins-----\n");
	memcpy(&gdt, ((void*)(*((u32*)&gdt_ptr[2]))),*((u16*)&gdt_ptr[0])+1);
	////////////

	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);//
	u32* p_gdt_base = (u32*)(&gdt_ptr[2]);//
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;//
	*p_gdt_base = (u32*)(&gdt);//
	
	/*u16 limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	u32 base = (u32)(&gdt);
	gdt_ptr[0] = *(u8*)(&limit);
	gdt_ptr[1] = *((u8*)(&limit) + 1);
	gdt_ptr[2] = *(u8*)(&base);
	gdt_ptr[3] = *((u8*)(&base)+1);
	gdt_ptr[4] = *((u8*)(&base)+2);
	gdt_ptr[5] = *((u8*)(&base)+3);*/
}
