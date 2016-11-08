;Context switch function, its c prototype is the following
;void switch_task(task_regs_t * curr_regs, task_regs_t * next_regs);
;Basically, it saves current cpu registers to 'curr_regs', and then load 'next_regs' to current cpu registers
; Equivalent c prototype void regs_switch(context_t * regs);

global user_regs_switch
global kernel_regs_switch
user_regs_switch:
    ; Load general registers
    ; Skip return address, and get the pointer regs
    mov ebp, [esp + 4]
    mov ecx, [ebp + 4]
    mov edx, [ebp + 8]
    mov ebx, [ebp + 12]
    mov esi, [ebp + 24]
    mov edi, [ebp + 28]
    ; load eflags
    mov eax, [ebp + 32]
    push eax
    popfd
    ; Right now, eax, ebp, esp are not restored yet

    ; Enter usermode from here(make sure the registers are restored correctly for the user process !)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push 0x23
    ; Push user esp
    mov eax, [ebp + 16]
    push eax
    pushfd
    push 0x1b
    ; Push eip
    mov eax, [ebp + 40]
    push eax
    ; Enter usermode from here(make sure the registers are restored correctly for the user process !)

    ; Load eax here
    mov eax, [ebp + 0]
    ; Now, restore ebp
    mov ebp, [ebp + 20]
    sti
    iret

kernel_regs_switch:
    ; Load general registers
    ; Skip return address, and get the pointer regs
    mov ebp, [esp + 4]
    mov ecx, [ebp + 4]
    mov edx, [ebp + 8]
    mov ebx, [ebp + 12]
    mov esi, [ebp + 24]
    mov edi, [ebp + 28]
    ; load eflags
    mov eax, [ebp + 32]
    push eax
    popfd
    ; Right now, eax, ebp, esp are not restored yet

    ; Enter usermode from here(make sure the registers are restored correctly for the user process !)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push 0x10
    ; Push user esp
    mov eax, [ebp + 16]
    push eax
    pushfd
    push 0x08
    ; Push eip
    mov eax, [ebp + 40]
    push eax
    ; Enter usermode from here(make sure the registers are restored correctly for the user process !)

    ; Load eax here
    mov eax, [ebp + 0]
    ; Now, restore ebp
    mov ebp, [ebp + 20]
    sti
    iret

