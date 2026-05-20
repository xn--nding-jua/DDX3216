echo off
cls

echo Start simulation...
rem "C:\Program Files\qemu\qemu-system-i386.exe" -drive format=raw,file=bootdisk.bin,if=floppy
"C:\Program Files\qemu\qemu-system-i386.exe" -drive format=raw,file=bootdisk.bin,if=ide,index=0,media=disk
echo Done
echo .
