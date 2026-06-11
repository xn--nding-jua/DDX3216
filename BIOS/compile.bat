echo off
cls
echo Building x86 BIOS for the Behringer DDX3216...
echo ===============================================
echo .

echo Step 1/3: Compiling sourcecode...
setlocal enabledelayedexpansion
set "CC=C:\Programme2\i686-elf-tools-windows\bin\i686-elf-gcc.exe"
set "LD=C:\Programme2\i686-elf-tools-windows\bin\i686-elf-ld.exe"
set "OBJCOPY=C:\Programme2\i686-elf-tools-windows\bin\i686-elf-objcopy.exe"
set "CFLAGS=-march=i386 -m16 -O0 -ffreestanding -fno-toplevel-reorder -fno-stack-protector -mpreferred-stack-boundary=2 -mno-80387 -nostdlib -fno-pic"

if not exist obj md obj
if not exist bin md bin

echo Compiling tiny8086 BASIC...
cd tiny8086basic
c:\Programme2\nasm-3.01\nasm.exe basic.asm -o ..\bin\basic.bin
cd ..

set "OBJECT_FILES="
for %%F in (*.s *.c) do (
    echo Compiling %%F...
    %CC% %CFLAGS% "%%F" -o "obj\%%~nF.o" -c
    set "OBJECT_FILES=!OBJECT_FILES! obj\%%~nF.o"
)
echo .

echo Step 2/3: Linking project...
echo %OBJECT_FILES%
%LD% -m elf_i386 --no-check-sections -T bios.ld %OBJECT_FILES% -o obj\bios.elf
echo .

echo Step 3/3: Creating binary-file...
%OBJCOPY% -O binary obj\bios.elf bin\bios.bin
echo Done
echo "======================================="
echo "   ____  ______  ___________  _  __    "
echo "  |  _ \|  _ \ \/ /___ /___ \/ |/ /_   "
echo "  | | | | | | \  /  |_ \ __) | | '_ \  "
echo "  | |_| | |_| /  \ ___) / __/| | (_) | "
echo "  |____/|____/_/\_\____/_____|_|\___/  "
echo "  x86 BIOS for Behringers DDX3216      "
echo "======================================="
