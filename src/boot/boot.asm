[bits 32]

; --- Constantes du Multiboot 1 ---
MB_MAGIC      equ 0x1BADB002        ; Magic Number
MB_ALIGN      equ 1 << 0            ; Align module on page (4 ko)
MB_MEMINFO    equ 1 << 1            ; memory card for the os
MB_FLAGS      equ MB_ALIGN | MB_MEMINFO
MB_CHECKSUM   equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

; kernel pile : 16 Ko, .bss
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kmain

_start:
    ; Grub dont config the pil; esp point ot the high
    mov esp, stack_top

    ; eax = magic GRUB (0x2BADB002), ebx = point to the info struct
    mov edi, eax
    mov esi, ebx
    cmp edi, 0x2BADB002
    jne .no_multiboot
    call check_long_mode
    ; same as vga c | (color << 8)
    ; O = 0x4F | color = 0x2F
    ; K = 0x4B | color = 0x2F4B
    mov dword [0xb8000], 0x2f4b2f4f
    cli

.halt:
    hlt
    jmp .halt

.no_multiboot:
    mov al, '0'
    jmp error

error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte [0xb800a], al

.stop:
    cli
    hlt
    jmp .stop

; Returns if the CPU supports long mode (64 bits), otherwise stops with error '2'.
; cpuid overwrites eax, ebx, ecx and edx: edi (magic) and esi (info) are not touched.
check_long_mode:
    ; is the extended cpuid range available ?
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode            ; unsigned jump: 0x80000001 is negative when signed

    ; extended features: bit 29 of edx = long mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret

.no_long_mode:
    mov al, '2'
    jmp error
