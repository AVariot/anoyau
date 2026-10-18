; gdt.asm - my GDT (Global Descriptor Table)
;
; What it is:
;   In protected mode, cs/ds/es/fs/gs/ss don't contain an address anymore.
;   They contain a selector = an index in a table (the GDT). Each entry is
;   8 bytes and says where the segment starts, its size and what it can do
;   (code or data, ring 0 or 3, read/write).
;
;       selector 0x08 -> entry 1 (0x08 / 8) -> base, limit, access
;
; Why I need my own:
;   GRUB already gave me a GDT to boot, but I don't know where it is and it can
;   be overwritten by my kernel. Later I also need entries it doesn't have
;   (user mode ring 3, TSS). So I set up mine right at the start.
;
; Why flat segments:
;   I won't use segmentation to cut the memory, paging will do it later.
;   So every segment covers everything: base 0, limit 4 GiB.
;   (limit 0xFFFFF * 4 KiB granularity = 4 GiB)
;   The segments are only here to have ring 0 / ring 3.
;
; Table:
;   0  0x00  null (mandatory)
;   1  0x08  kernel code
;   2  0x10  kernel data
;   (user code/data and TSS later)
;
; How to load it (3 steps):
;   1. lgdt: tell the CPU where the table is. Nothing changes yet, the
;      segment registers still have GRUB's selectors.
;   2. reload ds, es, fs, gs, ss with 0x10 (simple mov).
;   3. reload cs with 0x08. mov doesn't work on cs, only a far jump does.
;
; If a selector is wrong -> exception -> no IDT -> triple fault -> reboot loop.
;
; From C: void gdt_init(void);

[bits 32]

; dw = define word        = 16 bits
; db = define byte        =  8 bits
; dd = define double word = 32 bits
; dq = define quad word   = 64 bits

; One descriptor (8 bytes), fields in this order:
;   limit[0:15] | base[0:15] | base[16:23] | access | flags + limit[16:19] | base[24:31]

section .data
gdt_start:
    ; entry 0, selector 0x00: null descriptor
    dq 0x0000000000000000

    ; entry 1, selector 0x08: kernel code, 0 -> 4 GiB
    dw 0xffff       ; limit [0:15]
    dw 0x0          ; base [0:15]
    db 0x0          ; base [16:23]
    db 10011010b    ; access: present, ring 0, code/data, executable, readable
    db 11001111b    ; flags: 4 KiB granularity, 32 bit | limit [16:19] = 0xF
    db 0x0          ; base [24:31]

    ; entry 2, selector 0x10: kernel data, 0 -> 4 GiB
    dw 0xffff       ; limit [0:15]
    dw 0x0          ; base [0:15]
    db 0x0          ; base [16:23]
    db 10010010b    ; access: present, ring 0, code/data, not executable, writable
    db 11001111b    ; flags: 4 KiB granularity, 32 bit | limit [16:19] = 0xF
    db 0x0          ; base [24:31]
gdt_end:

; what lgdt reads (6 bytes)
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; size of the table - 1
    dd gdt_start                ; address of the table

KERNEL_CODE_SEL equ 0x08
KERNEL_DATA_SEL equ 0x10

section .text
global gdt_init

; void gdt_init(void)
gdt_init:
    lgdt [gdt_descriptor]

    ; data registers: mov through ax
    mov ax, KERNEL_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; cs: far jump to the next line
    jmp KERNEL_CODE_SEL:.reload_cs
.reload_cs:
    ret
