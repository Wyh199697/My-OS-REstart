org	0100h

BaseOfStack	equ	0100h
BaseOfKernelFile	equ	08000h
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
	dec word [wRootDirSizeForLoop]
	mov ax,BaseOfKernelFile
	mov es,ax
	mov bx,OffsetOfKernelFile
	mov ax,[wSectorNo]
	mov cl,1
	call ReadSector

	mov si,KernelFileName
	mov di,OffsetOfKernelFile
	cld
	mov dx,10h
LABEL_SEARCH_FOR_KERNELBIN:
	cmp dx,0
	jz LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR
	dec dx
	mov cx,11
LABEL_CMP_FILENAME:
	cmp cx,0
	jz LABEL_FILENAME_FOUND
	dec cx
	lodsb
	cmp al,[es:di]
	jz LABEL_GO_ON
	jmp LABEL_DIFFERENT
LABEL_GO_ON:
	inc di
	jmp LABEL_CMP_FILENAME

LABEL_DIFFERENT:
	and di,0ffe0h
	add di,20h
	mov si,KernelFileName
	jmp LABEL_SEARCH_FOR_KERNELBIN

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add word[wSectorNo],1
	jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_KERNELBIN:
	mov dh,2
	call DispStr

	jmp $

LABEL_FILENAME_FOUND:
	mov ax,RootDirSectors ;ax=14
	and di,0fff0h
	push eax
	mov eax,[es:di+01ch]
	mov dword [dwKernelSize],eax
	pop eax

	add di,01ah
	mov cx,word[es:di]
	push cx
	add cx,ax
	add cx,DeltaSectorNo
	mov ax,BaseOfKernelFile
	mov es,ax
	mov bx,OffsetOfKernelFile
	mov ax,cx ;ax=首簇号+14+19-2

LABEL_GOON_LOADING_FILE:
	push ax
	push bx
	mov ah,0eh
	mov al,'.'
	mov bl,0fh
	int 10h
	pop bx
	pop ax

	mov cl,1
	call ReadSector
	pop ax ;ax=首簇号
	call GetFATEntry
	cmp ax,0fffh ;ax=下个簇号
	jz LABEL_FILE_LOADED
	push ax ;这里push ax，上面pop ax
	mov dx,RootDirSectors
	add ax,dx
	add ax,DeltaSectorNo
	add bx,[BPB_BytsPerSec] ;内存+512，加载新的扇区
	jmp LABEL_GOON_LOADING_FILE
LABEL_FILE_LOADED:
	call KillMotor
	mov dh,1
	call DispStr
	jmp $

;======================
wRootDirSizeForLoop	dw RootDirSectors
wSectorNo	dw	0
bOdd	db	0
dwKernelSize	dd	0
;==================================
;=================================
KernelFileName	db "KERNEL  BIN",0
MessageLength	equ		9
LoadMessage	db	"Loading  "
Message1	db	"Ready.   "
Message2	db	"No KERNEL"
;===================================
;===================================
DispStr:
	mov	ax, MessageLength
	mul	dh
	add	ax, LoadMessage
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h		; AH = 13,  AL = 01h
	mov	bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	add	dh, 3			; 从第 3 行往下显示
	int	10h			; int 10h
	ret
;======================================
;================================
ReadSector:
	push bp
	mov bp,sp
	sub esp,2 ;cl是一个字节，push指令不好用，并且需要保存bp，用esp-2代替push

	mov byte[bp-2],cl
	push bx
	mov bl,[BPB_SecPerTrk]
	div bl
	inc ah
	mov cl,ah
	mov dh,al
	shr al,1
	mov ch,al
	and dh,1
	pop bx
	mov dl,[BS_DrvNum]
.GoOnReading:
	mov ah,2
	mov al,byte[bp-2]
	int 13h
	jc	.GoOnReading
	add esp,2
	pop bp
	ret
;===========================
;=============================
GetFATEntry:
	push es
	push bx
	push ax
	mov ax,BaseOfKernelFile
	sub	ax,0100h
	mov es,ax
	pop ax
	mov byte[bOdd],0
	mov bx,3
	mul bx
	mov bx,2
	div bx
	cmp dx,0
	jz LABEL_EVEN
	mov byte[bOdd],1
LABEL_EVEN:
	xor dx,dx
	mov bx,[BPB_BytsPerSec]
	div bx
	push dx
	mov bx,0
	add ax,SectorNoOfFAT1 ;ax=该fatentry在fat1表中的偏移表数+fat1表的扇区号
	mov cl,2
	call ReadSector
	pop dx
	add bx,dx ;关键就是这个dx，要是dx接近512，那么可能发生越界
	mov ax,[es:bx]
	cmp byte[bOdd],1
	jnz LABEL_EVEN_2
	shr ax,4
LABEL_EVEN_2:
	and ax,0fffh

LABEL_GET_FAT_ENTRY_OK:
	pop bx
	pop es
	ret
;============================
;===========================
KillMotor:
	push dx
	mov dx,03f2h
	mov al,0
	out dx,al
	pop dx
	ret
;===========================
