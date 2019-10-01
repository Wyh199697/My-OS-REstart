org	0100h


	jmp LABEL_START

%include "fat12hdr.inc"
%include "load.inc"
%include "pm.inc"


;====================================
LABEL_GDT:			Descriptor	0,	0,	0
LABEL_DESC_FLAT_C:	Descriptor	0,	0fffffh,	DA_CR|DA_32|DA_LIMIT_4K
LABEL_DESC_FLAT_RW:	Descriptor	0,	0fffffh,	DA_DRW|DA_32|DA_LIMIT_4K
LABEL_DESC_VIDEO:	Descriptor	0b8000h,	0fffffh,	DA_DRW|DA_DPL3

GdtLen	equ	$ - LABEL_GDT
GdtPtr	dw	GdtLen - 1
		dd	BaseOfLoaderPhyAddr + LABEL_GDT ;org 100h的存在，所以不用偏移之后加上100h
;====================================
;================================
SelectorFlatC	equ	LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW	equ	LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo	equ	LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3
;=====================
;==========================
BaseOfStack	equ	0100h
PageDirBase	equ	100000h
PageTblBase	equ	101000h
;===========================


LABEL_START:
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov sp,BaseOfStack

	mov dh,0
	call DispStrRealMode
		; 得到内存数
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf		; es:di 指向一个地址范围描述符结构(ARDS)
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h		; edx = 'SMAP'
	int	15h			; int 15h
	jc	.MemChkFail
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
	mov	dword [_dwMCRNumber], 0
.MemChkOK:

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
	call DispStrRealMode
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
	call DispStrRealMode

	lgdt [GdtPtr]

	cli

	in al,92h
	or al,00000010b
	out 92h,al

	mov eax,cr0
	or eax,1
	mov cr0,eax

	jmp dword SelectorFlatC:(BaseOfLoaderPhyAddr+LABEL_PM_START)
	

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
DispStrRealMode:
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
;==================================
[SECTION .s32]
ALIGN 32
[BITS 32]
LABEL_PM_START:
	mov ax,SelectorVideo
	mov gs,ax
	mov ax,SelectorFlatRW
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov ss,ax
	mov esp,TopOfStack
	
	push	szMemChkTitle
	call	DispStr
	add	esp, 4

	call	DispMemInfo
	call	SetupPaging

	mov	ah, 0Fh				; 0000: 黑底    1111: 白字
	mov	al, 'P'
	mov	[gs:((80 * 0 + 39) * 2)], ax	; 屏幕第 0 行, 第 39 列
	jmp	$


%include	"lib.inc"


; 显示内存信息 --------------------------------------------------------------
DispMemInfo:
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber];for(int i=0;i<[MCRNumber];i++)//每次得到一个ARDS
.loop:				  ;{
	mov	edx, 5		  ;  for(int j=0;j<5;j++)//每次得到一个ARDS中的成员
	mov	edi, ARDStruct	  ;  {//依次显示:BaseAddrLow,BaseAddrHigh,LengthLow
.1:				  ;               LengthHigh,Type
	push	dword [esi]	  ;
	call	DispInt		  ;    DispInt(MemChkBuf[j*4]); // 显示一个成员
	pop	eax		  ;
	stosd			  ;    ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4		  ;
	dec	edx		  ;
	cmp	edx, 0		  ;
	jnz	.1		  ;  }
	call	DispReturn	  ;  printf("\n");
	cmp	dword [dwType], 1 ;  if(Type == AddressRangeMemory)
	jne	.2		  ;  {
	mov	eax, [dwBaseAddrLow];
	add	eax, [dwLengthLow];
	cmp	eax, [dwMemSize]  ;    if(BaseAddrLow + LengthLow > MemSize)
	jb	.2		  ;
	mov	[dwMemSize], eax  ;    MemSize = BaseAddrLow + LengthLow;
.2:				  ;  }
	loop	.loop		  ;}
				  ;
	call	DispReturn	  ;printf("\n");
	push	szRAMSize	  ;
	call	DispStr		  ;printf("RAM size:");
	add	esp, 4		  ;
				  ;
	push	dword [dwMemSize] ;
	call	DispInt		  ;DispInt(MemSize);
	add	esp, 4		  ;

	pop	ecx
	pop	edi
	pop	esi
	ret
; ---------------------------------------------------------------------------

; 启动分页机制 --------------------------------------------------------------
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h	; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx		; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push	ecx		; 暂存页表个数

	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PageDirBase	; 此段首地址为 PageDirBase
	xor	eax, eax
	mov	eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的
	loop	.1

	; 再初始化所有页表
	pop	eax			; 页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov	edi, PageTblBase	; 此段首地址为 PageTblBase
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW
.2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2

	mov	eax, PageDirBase
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .3
.3:
	nop

	ret
; 分页机制启动完毕 ----------------------------------------------------------


; SECTION .data1 之开始 ---------------------------------------------------------------------------------------------
[SECTION .data1]

ALIGN	32

LABEL_DATA:
; 实模式下使用这些符号
; 字符串
_szMemChkTitle:	db "BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:	db "RAM size:", 0
_szReturn:	db 0Ah, 0
;; 变量
_dwMCRNumber:	dd 0	; Memory Check Result
_dwDispPos:	dd (80 * 6 + 0) * 2	; 屏幕第 6 行, 第 0 列
_dwMemSize:	dd 0
_ARDStruct:	; Address Range Descriptor Structure
  _dwBaseAddrLow:		dd	0
  _dwBaseAddrHigh:		dd	0
  _dwLengthLow:			dd	0
  _dwLengthHigh:		dd	0
  _dwType:			dd	0
_MemChkBuf:	times	256	db	0
;
;; 保护模式下使用这些符号
szMemChkTitle		equ	BaseOfLoaderPhyAddr + _szMemChkTitle
szRAMSize		equ	BaseOfLoaderPhyAddr + _szRAMSize
szReturn		equ	BaseOfLoaderPhyAddr + _szReturn
dwDispPos		equ	BaseOfLoaderPhyAddr + _dwDispPos
dwMemSize		equ	BaseOfLoaderPhyAddr + _dwMemSize
dwMCRNumber		equ	BaseOfLoaderPhyAddr + _dwMCRNumber
ARDStruct		equ	BaseOfLoaderPhyAddr + _ARDStruct
	dwBaseAddrLow	equ	BaseOfLoaderPhyAddr + _dwBaseAddrLow
	dwBaseAddrHigh	equ	BaseOfLoaderPhyAddr + _dwBaseAddrHigh
	dwLengthLow	equ	BaseOfLoaderPhyAddr + _dwLengthLow
	dwLengthHigh	equ	BaseOfLoaderPhyAddr + _dwLengthHigh
	dwType		equ	BaseOfLoaderPhyAddr + _dwType
MemChkBuf		equ	BaseOfLoaderPhyAddr + _MemChkBuf


; 堆栈就在数据段的末尾
StackSpace:	times	1024	db	0
TopOfStack	equ	BaseOfLoaderPhyAddr + $	; 栈顶
; SECTION .data1 之结束 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
