[bits 32]
extern isr_handler

section .text

; Common body of every stub. %1 = vector number.
%macro ISR_BODY 0
    push eax
    push ecx
    push edx
    push ebx
    push esp
    push ebp
    push esi
    push edi

    push esp                 ; argument of isr_handler(int num)
    call isr_handler
    add esp, 4              ; caller removes its argument

    pop edi
    pop esi
    pop ebp
    add esp, 4              ; skip the saved esp (same as popa)
    pop ebx
    pop edx
    pop ecx
    pop eax
    add esp, 8
%endmacro

; Exceptions where the CPU does NOT push an error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0
    push %1
    ISR_BODY
    iret
%endmacro

; Exceptions where the CPU pushes an error code
%macro ISR_ERR 1
global isr%1
isr%1:
    push %1
    ISR_BODY
    iret
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

; Vector for slave irq
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47

; isr_stub_table[i] = address of isr<i>
section .data
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 48
    dd isr %+ i
%assign i i+1
%endrep
