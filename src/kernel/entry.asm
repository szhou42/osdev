; Some constants used for multiboot header
; No need to understand, they are just a bunch of flags and magic values for the bootloader to find and recognize it as a multibootkernel
MBALIGN     equ  1<<0
MEMINFO     equ  1<<1
FLAGS       equ  MBALIGN | MEMINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)
; Some constants for loading higher half kernel
VM_BASE     equ 0xC0000000
PDE_INDEX   equ (VM_BASE >> 22)
PSE_BIT     equ 0x00000010
PG_BIT      equ 0x80000000

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .data
align 4096
global TEMP_PAGE_DIRECTORY
TEMP_PAGE_DIRECTORY:
    ; Map the first 4mb physical memory to first 4mb virtual memory. Otherwise, when paging is enabled, eip points to, 0x100004 for example, and MMU is not gonna know how to translate 
    ; this address into phsyical mem address, because our PDE doesn't tell MMU how to find it.
    dd 0x00000083
    times(PDE_INDEX - 1) dd 0
    dd 0x00000083
    times(1024 - PDE_INDEX - 1) dd 0 

; Our initial stack
section .initial_stack, nobits
align 4
stack_bottom:
    ; 1mb of uninitialized data(1024*1024=104856)
    resb 104856
stack_top:

; Kernel entry
section .text
global kernel_entry
global low_kernel_entry
low_kernel_entry equ (kernel_entry - VM_BASE)
kernel_entry:
    ; update page directory address, since eax and ebx is in use, have to use ecx or other register
    mov ecx, (TEMP_PAGE_DIRECTORY - VM_BASE)
    mov cr3, ecx

    ; Enable 4mb pages
    mov ecx, cr4;
    or ecx, PSE_BIT
    mov cr4, ecx

    ; Set PG bit, enable paging
    mov ecx, cr0
    or ecx, PG_BIT
    mov cr0, ecx

    ; Why not just jmp higher_half ? If you do that, that will be a relative jmp, so u r jumping to virtual memory around 0x100000, which is fine since we have identity mapped earlier
    ; but we also want to change the eip(now point to somewhere around 0x100000) to somewhere around 0xc0100000, so we need to get the absolute address of higher half into ecx, and jmp ecx
    lea ecx, [higher_half]
    jmp ecx
higher_half:
    ; Unmap the first 4mb physical mem, because we don't need it anymore. Flush the tlb too
    mov dword[TEMP_PAGE_DIRECTORY], 0
    invlpg[0]

    mov esp, stack_top
    extern kmain
    ; Upon entry to the os, the bootloader has put a pointer to multiboot information structure in ebx, we can pass it into our kmain(), but we may or may not need to use it
    push ebx
    ; When control is transfer to the c code, we can throw away the old pageing directory structure and use our own, remember to clear pse bit in cr4 though :)
    call kmain
; If kmain return, just keep looping...
loop:
    jmp loop
