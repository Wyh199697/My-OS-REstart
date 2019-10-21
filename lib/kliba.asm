;[SECTION .data]
;disp_pos	dd	0
%include "sconst.inc"

extern disp_pos

[SECTION .text]

global disp_str
global out_byte
global disp_color_str
global  enable_irq
global  disable_irq
;========================
disp_str:
	push ebp
	mov ebp,esp

	mov esi,[ebp+8]
	mov edi,[disp_pos]
	mov ah,0fh
.1:
	lodsb
	test al,al
	jz .2
	cmp al,0ah
	jnz .3
	push eax
	mov eax,edi
	mov bl,160
	div bl
	and eax,0ffh
	inc eax
	mov bl,160
	mul bl
	mov edi,eax
	pop eax
	jmp .1
.3:
	mov [gs:edi],ax
	add edi,2
	jmp .1
.2:	
	mov [disp_pos],edi
	pop ebp
	ret
;=============================
out_byte:	;需要修改,想不通
	mov dx,[esp+4]
	mov al,[esp+4+4]
	out dx,al
	nop
	nop
	ret
;=============================
in_byte:
	mov edx,[esp+4]
	xor eax,eax
	in al,dx
	nop
	nop
	ret
;==================================
disp_color_str:
	push ebp
	mov ebp,esp

	mov esi, [ebp + 8]
	mov edi, [disp_pos]
	mov ah, [ebp + 12]
.1:
	lodsb
	test al,al
	jz	.2
	cmp al, 0ah
	jnz .3
	push eax
	mov eax,edi
	mov bl,160
	div bl
	and eax,0ffh
	inc eax
	mov bl,160
	mul bl
	mov edi,eax
	pop eax
	jmp .1
.3:
	mov [gs:edi],ax
	add edi,2
	jmp .1
.2
	mov [disp_pos],edi
	pop ebp
	ret
;=================================
; ========================================================================
;                  void disable_irq(int irq);
; ========================================================================
; Disable an interrupt request line by setting an 8259 bit.
; Equivalent code:
;	if(irq < 8)
;		out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) | (1 << irq));
;	else
;		out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) | (1 << irq));
disable_irq:
        mov     ecx, [esp + 4]          ; irq
        pushf
        cli
        mov     ah, 1
        rol     ah, cl                  ; ah = (1 << (irq % 8))
        cmp     cl, 8
        jae     disable_8               ; disable irq >= 8 at the slave 8259
disable_0:
        in      al, INT_M_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_M_CTLMASK, al       ; set bit at master 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
disable_8:
        in      al, INT_S_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_S_CTLMASK, al       ; set bit at slave 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
dis_already:
        popf
        xor     eax, eax                ; already disabled
        ret

; ========================================================================
;                  void enable_irq(int irq);
; ========================================================================
; Enable an interrupt request line by clearing an 8259 bit.
; Equivalent code:
;       if(irq < 8)
;               out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) & ~(1 << irq));
;       else
;               out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) & ~(1 << irq));
;
enable_irq:
        mov     ecx, [esp + 4]          ; irq
        pushf
        cli
        mov     ah, ~1
        rol     ah, cl                  ; ah = ~(1 << (irq % 8))
        cmp     cl, 8
        jae     enable_8                ; enable irq >= 8 at the slave 8259
enable_0:
        in      al, INT_M_CTLMASK
        and     al, ah
        out     INT_M_CTLMASK, al       ; clear bit at master 8259
        popf
        ret
enable_8:
        in      al, INT_S_CTLMASK
        and     al, ah
        out     INT_S_CTLMASK, al       ; clear bit at slave 8259
        popf
        ret
