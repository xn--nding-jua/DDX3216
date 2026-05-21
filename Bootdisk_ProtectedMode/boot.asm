; Bootsector for the AMD SC300 within the Behringer DDX3216
; (c) 2026 Chris Noeding, christian@noeding-online.de
; https://chrisdevblog.com
;
; This assembler-code fits into the first 512 bytes
; of a disk-image (first sector = bootsector). It outputs
; a small "Hello World" and copies the next 2 sectors
; via BIOS INT13h function into the RAM and jumps to
; the C-Main-Function located in sector 2
;

SECTION .text

USE16                  ; same as "BITS 16"
CPU 386                ; allow cpu-instructions up to 386

[extern main]
global boot

; this function is called by BIOS
boot:
	cli                ; no interrupts until stack is OK
	
    mov ax, 0x07C0     ; place stack behind the bootloader
    mov ss, ax
    mov sp, 0x07C0     ; set stackpointer

	sti                ; enable interrupts

    mov si, message    ; point SI register to message
    mov ah, 0x0e       ; set higher bits to the display character command
.loop:
    lodsb              ; load the character within the AL register, and increment SI
    cmp al, 0          ; is the AL register a null byte?
    je .start_c_code   ; jump to halt
    int 0x10           ; trigger video services interrupt
    jmp .loop          ; loop again
.start_c_code:
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

	; switch to 32-bit protected mode
	cli                ; disable interrupts
	lgdt [gdt_descriptor] ; load GDT
	
	; now enable protected mode in CR0
	mov eax, cr0
    or eax, 0x1        ; set Bit 0 (PE-Bit)
    mov cr0, eax
	
	; far-jump to code-segment in GDT
	jmp 0x08:init_pm

[bits 32]
init_pm:
	; switch all data-segment-registers to new 32-bit segment (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

	; setup new 32-bit stack to 90kB
    mov ebp, 0x90000
    mov esp, ebp

	; start C-code
    jmp main

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

; messages
message:
    db "Hello World from Assembler-Code!", 10, 13, 0
error_message:
    db "Error loading data from sector 2!", 10, 13, 0

; global descriptor table (GDT)
gdt_start:
    ; mandatory null-descriptor
    dd 0x0
    dd 0x0

    ; code-segment (Base=0x0, Limit=0xFFFFF, 4KB-Granularity = 4GB Flat)
    dw 0xFFFF    ; Limit (Bits 0-15)
    dw 0x0       ; Base (Bits 0-15)
    db 0x0       ; Base (Bits 16-23)
    db 10011010b ; Access Byte (Code, Callable, Read)
    db 11001111b ; Flags (32-Bit Mode, 4KB Granularity) + Limit (16-19)
    db 0x0       ; Base (Bits 24-31)

    ; 3. Daten-Segment (Base=0x0, Limit=0xFFFFF, 4KB-Granularity = 4GB Flat)
    dw 0xFFFF    ; Limit (Bits 0-15)
    dw 0x0       ; Base (Bits 0-15)
    db 0x0       ; Base (Bits 16-23)
    db 10010010b ; Access Byte (Daten, Writable)
    db 11001111b ; Flags (32-Bit Modus, 4KB Granularity) + Limit (16-19)
    db 0x0       ; Base (Bits 24-31)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size of GDT
    dd gdt_start               ; Startaddress of GDT
