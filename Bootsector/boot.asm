SECTION .text

USE16                  ; same as "BITS 16"
CPU 386                ; allow cpu-instructions up to 386

[extern main]
global boot

boot:
    mov si, message    ; point SI register to message
    mov ah, 0x0e       ; set higher bits to the display character command

.loop:
    lodsb              ; load the character within the AL register, and increment SI
    cmp al, 0          ; is the AL register a null byte?
    je .start_c_code   ; jump to halt
    int 0x10           ; trigger video services interrupt
    jmp .loop          ; loop again

.start_c_code:
    mov ax, 0x07C0     ; place stack behind the bootloader
    mov ss, ax
    mov sp, 0x4000     ; set stackpointer
    jmp main

message:
    db "Hello World from Assembler-Code!", 10, 13, 0
