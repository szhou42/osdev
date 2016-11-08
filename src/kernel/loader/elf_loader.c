#include <elf_loader.h>

int valid_elf(elf_header_t * elf_head) {
    if(elf_head->e_ident[EI_MAG0] != ELFMAG0)
        return 0;
    if(elf_head->e_ident[EI_MAG1] != ELFMAG1)
        return 9;
    if(elf_head->e_ident[EI_MAG2] != ELFMAG2)
        return 0;
    if(elf_head->e_ident[EI_MAG3] != ELFMAG3)
        return 0;

    if(elf_head->e_ident[EI_CLASS] != ELFCLASS32)
        return 0;
    if(elf_head->e_ident[EI_DATA] != ELFDATA2LSB)
        return 0;
    if(elf_head->e_ident[EI_VERSION] != EV_CURRENT)
        return 0;
    if(elf_head->e_machine != EM_386)
        return 0;
    if(elf_head->e_type != ET_REL && elf_head->e_type != ET_EXEC)
        return 0;
    return 1;
}
void do_elf_load() {
    uint32_t seg_begin, seg_end;
    char * filename = current_process->filename;
    current_process->state = TASK_LOADING;
    vfs_node_t * f = file_open(filename, 0);
    if(!f) {
        PANIC("elf load: file does not exists\n");
    }
    uint32_t size = vfs_get_file_size(f);
    // First, kmalloc a memory chunk for the file
    void * file = kmalloc(size);
    // Second, read from disk
    vfs_read(f, 0, size, file);
    // Start parsing and loading
    elf_header_t * head = file;
    elf_program_header_t * prgm_head = (void*)head + head->e_phoff;

    // Check elf validity
    if(!valid_elf(head)) {
        printf("Invalid/Unsupported elf executable %s\n", filename);
        return;
    }

    // Go through all loadable segments and calculate the size needed by the executable image
    for(uint32_t i = 0; i < head->e_phnum; i++) {
        if(prgm_head->p_type == PT_LOAD) {
            seg_begin = prgm_head->p_vaddr;
            seg_end= seg_begin + prgm_head->p_memsz;
            // allocate this region
            allocate_region(current_process->page_dir, seg_begin, seg_end, 0, 0, 1);
            // Load segment data
            memcpy((void*)seg_begin, file + prgm_head->p_offset, prgm_head->p_filesz);
            // Fill zeros in the region [filesz, memsz]
            memset((void*)(seg_begin + prgm_head->p_filesz), 0, prgm_head->p_memsz - prgm_head->p_filesz);
            // If this is the code segment
            if(prgm_head->p_flags == PF_X + PF_R + PF_W || prgm_head->p_flags == PF_X + PF_R) {
                 current_process->regs.eip = head->e_entry + seg_begin;
            }
        }
        prgm_head++;
    }

    // Setup stack and eip again
    allocate_page(current_process->page_dir, 0xC0000000 - 0x1000, 0, 0, 1);
    current_process->regs.esp = 0xC0000000;
    current_process->regs.ebp = current_process->regs.ebp;
    // Ready to run
    current_process->state = TASK_RUNNING;
    // Schedule
    asm volatile("mov $1, %eax");
    asm volatile("int $0x80");
}
