echo off
del *.o
del *.elf
cls

echo Use NASM to create object-file
..\nasm-3.01\nasm.exe -f elf32 start.asm -o start.o
echo Done
echo .

echo Compile c-files with -m16 to force GCC to use the stack within the 64kB segment
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib bios.c -o bios.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib uart.c -o uart.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib lcd.c -o lcd.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib disk.c -o disk.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib timer.c -o timer.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib isr.c -o isr.o -c
echo Done
echo .

echo Link assembler-part and c-part together
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-ld.exe -m elf_i386 -T bios.ld start.o bios.o uart.o lcd.o disk.o timer.o isr.o -o bios.elf
echo Done
echo .

echo Create 64kB ROM-file
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-objcopy.exe -O binary bios.elf bios.bin
echo Done
echo .

pause
