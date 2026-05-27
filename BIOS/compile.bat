echo off
md obj
del obj\*.o
del obj\*.elf
cls

echo Compile assembler- and c-files...
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 start.S -o obj\start.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib bios.c -o obj\bios.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib uart.c -o obj\uart.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib lcd.c -o obj\lcd.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib disk.c -o obj\disk.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib timer.c -o obj\timer.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib isr.c -o obj\isr.o -c
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe -march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-omit-frame-pointer -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib helper.c -o obj\helper.o -c
echo Done
echo .

echo Link assembler-part and c-part together...
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-ld.exe -m elf_i386 --no-check-sections -T bios.ld obj\start.o obj\bios.o obj\uart.o obj\lcd.o obj\disk.o obj\timer.o obj\isr.o obj\helper.o -o obj\bios.elf
echo Done
echo .

echo Create 64kB ROM-file...
C:\Programme2\i686-elf-tools-windows\bin\i686-elf-objcopy.exe -O binary obj\bios.elf bios.bin
echo Done
echo .
