global switch_to_user
switch_to_user:
    ; Turn off interrupts
    ; Setup data segment(remember, a selector is an offset into the gdt table, user data is the 4th segment(counting from 0), so cs = 4 * 8 = 24 = 0x20, 0x20+3(rpl) = 0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23
    push esp
    pushfd
    ; code segment selector, rpl = 3
    push 0x1b
    lea eax, [user_start]
    push eax
    iretd
user_start:
    add esp, 4
    ret
