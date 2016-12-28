global sse_available
global sse_init

sse_available:
    mov eax, 0x1
    cpuid
    test edx, 1 << 25
    mov eax, 0x1
    jnz good
    mov eax, 0
good:
    ret


sse_init:
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax
    ret
