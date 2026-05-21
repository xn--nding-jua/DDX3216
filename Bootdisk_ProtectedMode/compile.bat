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

echo Compile c-files for 32-Bit ProtectedMode
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib main.c -o obj\main.o -c
echo Done
echo .

echo Link assembler-part and c-part together
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-ld.exe -m elf_i386 -T bootdisk.ld obj\boot.o obj\main.o -o obj\bootdisk.elf
echo Done
echo .

echo Create 512 Byte bootdisk-file
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-objcopy.exe -O binary obj\bootdisk.elf bootdisk.bin
echo Done
echo .

pause
