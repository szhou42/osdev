/* We are trying to load the kernel at a high virtual address 0xC0000000, which has many benefits(but grub is gonna load it to phsical address 1MB anyway).
 * So what we do, is we first let the linker link our kernel based on the assumption that the kernel is placed on 0xC0100000(which is not true), by doing ".=0xC0100000"
 * Then, we tell the linker the change the text/data/rodata/bss's LMA(load memory address) to somewhere around 0x100000, by doing .text ALIGN(4K) : AT(ADDR(.text)-0xC0000000). 
 * The grub see this LMA and do load our kernel to 0x100000.
 * OK, when our kernel finally gets to run, it immediately enable paging and map the memory around 0x100000 to 0xC0000000
 * Guess what ? Our kernel is now in higher half ! (Yeah, in the context of virtual memory, it is)
 * 
 * You may wonder why we even bother to do all this... why not just load the kernel at 3GB ??!
 * because... not all computers have 4gb of physical memory(one of the reason I can think of).
 * */
OUTPUT_FORMAT("elf32-i386")
ENTRY(low_kernel_entry)
SECTIONS
{
    /*Kernel binary starts at 3GB. This is VMA, virtual memory address, so now all the variable, code label is going to be reference based on this VMA
     * 
     * */
    . = 0xC0100000;
    /*
        *(.section name) means: all sections named "section name" in all input object files, please put them here
        multiboot is a section defined in entry.asm, it contains information for grub to find where our kernel entry point is

       use keyword AT to relocate text section, same for rodata, data and bss section
    */
    .text ALIGN(4K) : AT(ADDR(.text)-0xC0000000)
    {
        *(.multiboot)
        *(.text)
    }
    .rodata ALIGN (4K) : AT(ADDR(.rodata)-0xC0000000)
    {
        *(.rodata)
    }
     /*
        .bss is also data section, but it contains uninitialized data
        .common is also uninitialized data section
        .initial_stack is a section defined in entry.asm, as an initial stack for our kernel
    */
    .bss ALIGN(4K) : AT(ADDR(.bss)-0xC0000000)
    {
        *(.COMMON)
        *(.bss)
        *(.initial_stack)
    }
    .data ALIGN(4K) : AT(ADDR(.data)-0xC0000000)
    {
        *(.data)
    }

    /*
        Put a symbol end here, it tells us where all the kernel code/data ends, it means everything after 'end' can be used for something else
    */
    end = .;
}


