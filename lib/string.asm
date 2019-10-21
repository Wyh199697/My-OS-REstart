[SECTION .text]

global memcpy
global memset
global strcpy

memcpy:
	push ebp
	mov ebp,esp

	push esi
	push edi
	push ecx

	mov edi,[ebp + 8]
	mov esi,[ebp + 12]
	mov ecx,[ebp + 16]
.1:
	cmp ecx,0
	jz .2
	mov al,[ds:esi]
	inc esi
	mov byte [es:edi],al
	inc edi
	dec ecx
	jmp .1
.2:
	mov eax,[ebp+8]

	pop ecx
	pop edi
	pop esi
	mov esp,ebp
	pop ebp

	ret


memset:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	edx, [ebp + 12]	; Char to be putted
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	byte [edi], dl		; ┓
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回

strcpy:
	push    ebp
	mov     ebp, esp

	mov     esi, [ebp + 12] ; Source
	mov     edi, [ebp + 8]  ; Destination

.1:
	mov     al, [esi]               ; ┓
	inc     esi                     ; ┃
					; ┣ 逐字节移动
	mov     byte [edi], al          ; ┃
	inc     edi                     ; ┛

	cmp     al, 0           ; 是否遇到 '\0'
	jnz     .1              ; 没遇到就继续循环，遇到就结束

	mov     eax, [ebp + 8]  ; 返回值

	pop     ebp
	ret                     ; 函数结束，返回
