
; Some templates for exception handler(with/without err code)

%macro EXCEPTION_NOERR 1
    global exception%1
exception%1:
    cli
    push byte 0     ; place holder, so that stack would look as if it's an exception with err code
    push %1
    jmp temp_exception_handler
%endmacro

%macro EXCEPTION_ERR 1
    global exception%1
exception%1:
    cli
    push %1
    jmp temp_exception_handler
%endmacro

EXCEPTION_NOERR 0
EXCEPTION_NOERR 1
EXCEPTION_NOERR 2
EXCEPTION_NOERR 3
EXCEPTION_NOERR 4
EXCEPTION_NOERR 5
EXCEPTION_NOERR 6
EXCEPTION_NOERR 7
EXCEPTION_ERR 8
EXCEPTION_NOERR 9
EXCEPTION_ERR 10
EXCEPTION_ERR 11
EXCEPTION_ERR 12
EXCEPTION_ERR 13
EXCEPTION_ERR 14
EXCEPTION_NOERR 15
EXCEPTION_NOERR 16
EXCEPTION_NOERR 17
EXCEPTION_NOERR 18
EXCEPTION_NOERR 19
EXCEPTION_NOERR 20
EXCEPTION_NOERR 21
EXCEPTION_NOERR 22
EXCEPTION_NOERR 23
EXCEPTION_NOERR 24
EXCEPTION_NOERR 25
EXCEPTION_NOERR 26
EXCEPTION_NOERR 27
EXCEPTION_NOERR 28
EXCEPTION_NOERR 29
EXCEPTION_NOERR 30
EXCEPTION_NOERR 31
EXCEPTION_NOERR 128

extern final_exception_handler

temp_exception_handler:
    pusha                           ; push eax, ebx, ecx, edx, esi, edi, ebp, esp
    mov ax, ds
    push eax                        ; save ds
    
    mov ax, 0x10                    ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call final_exception_handler

    pop ebx                         ; restore original data segment
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 0x8                    ; rebalance stack(pop err code and exception number)

    sti                             ; re-enable interrupts
    iret
