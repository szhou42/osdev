%define KERNEL_DATA_SEGMENT 0x10
%define CODE_DATA_SEGMENT 0x08

[GLOBAL gdt_flush]
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, KERNEL_DATA_SEGMENT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp CODE_DATA_SEGMENT:flush 
flush:
    ret
