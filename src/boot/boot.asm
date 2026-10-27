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
align 4096
pml4:   resb 4096
pdpt:   resb 4096
pd:     resb 4096
stack_bottom:
    resb 16384
stack_top:

; Temporary 64-bit GDT, only used to enter long mode (gdt_init replaces it later)
section .rodata
align 8
gdt64:
    dq 0                        ; 0x00: null descriptor
    dq 0x00AF9A000000FFFF       ; 0x08: code, L=1 (64 bits), executable, ring 0
    dq 0x00CF92000000FFFF       ; 0x10: data, writable, ring 0
gdt64_ptr:
    dw gdt64_ptr - gdt64 - 1    ; size of the table - 1
    dd gdt64                    ; address of the table (6 bytes read by lgdt in 32-bit mode)

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
    call setup_page_tables
    call enable_paging

    ; load the 64-bit GDT, then a far jump reloads cs with the 64-bit code segment
    lgdt [gdt64_ptr]
    jmp 0x08:long_mode_start

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

setup_page_tables:
    mov eax, pdpt
    or  eax, 0b11               ; present | writable
    mov [pml4], eax

    mov eax, pd
    or  eax, 0b11
    mov [pdpt], eax

    xor ecx, ecx
.map:
    mov eax, 0x200000
    mul ecx                     ; edx:eax = ecx * 2 MiB
    or  eax, 0b10000011         ; present | writable | 2 MiB page
    mov [pd + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map
    ret

; Point cr3 to our tables, enable PAE, enable long mode in EFER, then turn paging on.
; Order matters: PAE and LME must be set before paging.
enable_paging:
    mov eax, pml4
    mov cr3, eax

    mov eax, cr4
    or  eax, 1 << 5             ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080         ; EFER, a model specific register
    rdmsr                       ; edx:eax = EFER
    or  eax, 1 << 8             ; LME: long mode enable
    wrmsr

    mov eax, cr0
    or  eax, 1 << 31            ; PG: paging
    mov cr0, eax
    ret

; From here on the code is 64 bits: the assembler must know it
bits 64
long_mode_start:
    mov ax, 0x10                ; data selector of gdt64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, stack_top          ; writing esp clears the upper half of rsp
    mov edi, edi                ; rdi = magic (zero-extended): 1st argument of kmain
    mov esi, esi                ; rsi = info  (zero-extended): 2nd argument of kmain
    call kmain

    ; kmain should never return
.halt:
    cli
    hlt
    jmp .halt
