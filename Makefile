ENTRYPOINT = 0x30400

ENTRYOFFSET = 0x400

ASM = nasm
DASM = ndisasm
CC = gcc
LD = ld
ASMBFLAGS = -I boot/include/
ASMKFLAGS = -I include/ -f elf
CFLAGS = -I include/ -m32 -c -fno-builtin -fno-stack-protector
LDFLAGS = -m elf_i386 -s -Ttext $(ENTRYPOINT) -e $(ENTRYOFFSET)

ORANGESBOOT = boot/boot.bin boot/loader.bin
ORANGESKERNEL = kernel/kernel.bin
OBJS = kernel/kernel.o kernel/start.o lib/string.o lib/kliba.o kernel/i8259.o kernel/global.o lib/klib.o kernel/protect.o kernel/main.o kernel/clock.o kernel/proc.o kernel/syscall.o kernel/keyboard.o kernel/tty.o kernel/console.o kernel/printf.o kernel/vsprintf.o
DASMOUTPUT = kernel.bin.asm


.PHONY : everything final image clean realclean disasm all buildimg

everything : $(ORANGESBOOT) $(ORANGESKERNEL)

all : realclean everything

final : all clean

image : final buildimg

clean : 
	rm -f $(OBJS)

realclean : 
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL)

disasm : 
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

buildimg : 
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel/kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/fat12hdr.inc boot/include/load.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/fat12hdr.inc boot/include/load.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<
	
$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(ORANGESKERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<
	
kernel/syscall.o : kernel/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/kliba.o : lib/kliba.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o : kernel/start.c include/type.h include/const.h include/protect.h include/string.h include/proto.h include/global.h include/proc.h include/tty.h include/console.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o: kernel/protect.c /usr/include/stdc-predef.h include/type.h \
 include/const.h include/protect.h include/global.h include/proto.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259.o : kernel/i8259.c include/type.h include/const.h include/protect.h include/proto.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/global.o: kernel/global.c include/type.h \
 include/const.h include/protect.h include/proto.h include/global.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<
	
lib/klib.o: lib/klib.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<

main.o: kernel/main.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<

clock.o: kernel/clock.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<
	
proc.o: kernel/clock.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<

keyboard.o: kernel/keyboard.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h include/keyboard.h include/keymap.h
	$(CC) $(CFLAGS) -o $@ $<

tty.o: kernel/tty.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h include/keyboard.h include/keymap.h
	$(CC) $(CFLAGS) -o $@ $<

console.o: kernel/console.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h include/keyboard.h include/keymap.h
	$(CC) $(CFLAGS) -o $@ $<

printf.o: kernel/printf.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h include/keyboard.h include/keymap.h
	$(CC) $(CFLAGS) -o $@ $<

vsprintf.o: kernel/vsprintf.c include/type.h \
 include/const.h include/protect.h include/proto.h include/string.h \
 include/global.h include/proc.h include/keyboard.h include/keymap.h
	$(CC) $(CFLAGS) -o $@ $<
