nasm boot.asm -o boot.bin
dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
nasm loader.asm -o loader.bin
sudo mount -o loop a.img /mnt/floppy
sudo cp loader.bin /mnt/floppy -v
sudo umount /mnt/floppy

nasm boot.asm -o boot.com
sudo mount -o loop pm.img /mnt/floppy
sudo cp boot.com /mnt/floppy/
sudo umount /mnt/floppy

sudo mv nasm.vim 1.vim
sudo mv asm.vim nasm.vim
sudo mv 1.vim asm.vim
