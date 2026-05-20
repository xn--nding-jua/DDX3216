echo off
md obj
del obj\*.o
del obj\*.elf
del *.bin
cls

echo Use NASM to create object-file
c:\Programme2\nasm-3.01\nasm.exe -f elf32 boot.asm -o obj\boot.o
echo Done
echo .

echo Compile c-files with -m16 to force GCC to use the stack within the 64kB segment
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386 -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib main.c -o obj\main.o -c
echo Done
echo .

echo Link assembler-part and c-part together
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-ld.exe -m elf_i386 -T bootsector.ld obj\boot.o obj\main.o -o obj\bootsector.elf
echo Done
echo .

echo Create 512 Byte Bootsector-file
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-objcopy.exe -O binary obj\bootsector.elf bootsector.bin
echo Done
echo .

pause
