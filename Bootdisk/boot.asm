SECTION .text

USE16                  ; same as "BITS 16"
CPU 386                ; allow cpu-instructions up to 386

[extern main]
global boot

; this function is called by BIOS
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

	mov ah, 0x02       ; call BIOS-Function: "read sector"
    mov al, 0x02       ; number of sectors (here: 2 -> 2 * 512 Byte = 1024 Byte)
    mov ch, 0x00       ; cylinder 0
    mov dh, 0x00       ; head 0
    mov cl, 0x02       ; set start-sector to #2
	
	; BIOS sets boot-drive into register DL, so we don't change it
    
	; load data to ES:BX. ES is 0, set BX to 0x7E00
    mov bx, 0x7E00     ; the address after 0x7C00
    int 0x13           ; call BIOS-interrupt to start copy-process
    jc disk_error      ; if carry-flag is set an error has occured

    jmp main           ; start C-main-function

halt:
    hlt
    jmp halt
	
disk_error:
    mov si, error_message    ; point SI register to message
    mov ah, 0x0e       ; set higher bits to the display character command
.loop_err:
    lodsb              ; load the character within the AL register, and increment SI
    cmp al, 0          ; is the AL register a null byte?
    je halt            ; jump to halt
    int 0x10           ; trigger video services interrupt
    jmp .loop_err      ; loop again

message:
    db "Hello World from Assembler-Code!", 10, 13, 0
error_message:
    db "Error loading data from sector 2!", 10, 13, 0
