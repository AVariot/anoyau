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
    push ebx
    push eax
    call kmain

    cli
.halt:
    hlt
    jmp .halt
