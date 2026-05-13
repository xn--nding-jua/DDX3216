echo off
md obj
del obj\*.o
del obj\*.elf
cls

echo Use NASM to create object-file
c:\Programme2\nasm-3.01\nasm.exe -f elf32 start.asm -o obj\start.o
echo Done
echo .

echo Compile c-files with -m16 to force GCC to use the stack within the 64kB segment
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib bios.c -o obj\bios.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib uart.c -o obj\uart.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib lcd.c -o obj\lcd.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib disk.c -o obj\disk.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib timer.c -o obj\timer.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -m32 -march=i386  -m16 -ffreestanding -fno-stack-protector -mno-80387 -nostdlib isr.c -o obj\isr.o -c
echo Done
echo .

echo Link assembler-part and c-part together
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-ld.exe -m elf_i386 --no-check-sections -T bios.ld obj\start.o obj\bios.o obj\uart.o obj\lcd.o obj\disk.o obj\timer.o obj\isr.o -o obj\bios.elf
echo Done
echo .

echo Create 64kB ROM-file
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-objcopy.exe -O binary obj\bios.elf bios.bin
echo Done
echo .

pause
