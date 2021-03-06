nasm boot.asm -o boot.bin
dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc  boot.bin不需要fat12头，他位于引导扇区
nasm loader.asm -o loader.bin
nasm -f elf kernel.asm -o kernel.o
ld -s -Ttext 0x30400 -o kernel.bin kernel.o
sudo mount -o loop a.img /mnt/floppy
sudo cp loader.bin /mnt/floppy -v  loader.bin和kernel.bin需要fat12头，便于载入内存，所以命令只需要复制就可以了
sudo cp kernel.bin /mnt/floppy -v
sudo umount /mnt/floppy

nasm boot.asm -o boot.com
sudo mount -o loop pm.img /mnt/floppy
sudo cp boot.com /mnt/floppy/
sudo umount /mnt/floppy

sudo mv nasm.vim 1.vim
sudo mv asm.vim nasm.vim
sudo mv 1.vim asm.vim

nasm -f elf kernel.asm -o kernel.o
gcc -m32 -c bar.c -o bar.o
ld -m elf_i386 -s -o kernel.bin kernel.o

为什么内核加载进来之后还需要再在内存中移动一次呢，因为后面都是c和汇编一起用，都是32位代码，编译链接出来的都是elf文件，所以要根据elf文件再次移动一次内核。

需要注意的是，#ifndef起到的效果是防止一个源文件两次包含同一个头文件，而不是防止两个源文件包含同一个头文件。

nasm boot.asm -o boot.bin
dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
nasm loader.asm -o loader.bin
nasm -f elf kernel.asm -o kernel.o
nasm -f elf string.asm -o string.o
nasm -f elf kliba.asm -o kliba.o
gcc -m32 -c start.c -o start.o
ld -s -Ttext 0x30400 -o kernel.bin start.o kernel.o string.o kliba.o
sudo mount -o loop a.img /mnt/floppy
sudo cp loader.bin /mnt/floppy -v
sudo cp kernel.bin /mnt/floppy -v
sudo umount /mnt/floppy

gcc -M start.c -I include

