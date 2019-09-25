org	0100h

BaseOfStack	equ	0100h
BaseOfKernelFIle	equ	08000h
OffsetOfKernelFile	equ	0h


	jmp LABEL_START

%include "fat12hdr.inc"

LABEL_START:
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov sp,BaseOfStack

	mov dh,0
	call DispStr

	mov word [wSectorNo], SectorNoOfRootDirectory
	xor ah,ah
	xor dl,dl
	int 13h
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
	cmp word [wRootDirSizeForLoop],0
	jz LABEL_NO_KERNELBIN
	mov ax,BaseOfKernelFile
	mov es,ax
	mov bx,OffsetOfKernelFile
	mov ax,[wSectorNo]
	mov cl,1
	call ReadSector
