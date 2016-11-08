; Interrupt template, 2 arguments, first is irq number(0 to 15), second is the interrupt number it's mapped to
%macro IRQ 2
    global irq%1
irq%1:
    cli
    push byte 0 ; always push a dummy err code so that we can reuse the register_t struct
    push byte %2
    jmp temp_irq_handler
%endmacro


IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern final_irq_handler

temp_irq_handler:
    pusha                           ; push eax, ebx, ecx, edx, esi, edi, ebp, esp
    mov ax, ds
    push eax                        ; save ds

    mov ax, 0x10                    ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call final_irq_handler

    pop ebx                         ; restore original data segment
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 0x8                    ; rebalance stack(pop err code and exception number)

    sti                             ; re-enable interrupts
    iret
