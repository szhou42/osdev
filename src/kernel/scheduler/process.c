#include <process.h>


list_t * process_list;
pcb_t * current_process;
pcb_t * last_process;

uint32_t prev_jiffies;
pid_t curr_pid;
// Whenever interrupt/exception/syscall(which is soft exception) happens, we should store the context from previous process in here, so that scheduler can use it
register_t * saved_context;

pid_t allocate_pid() {
    return curr_pid++;
}
/*
 * Yeah, context switch, what else to put here ?...
 * */
void context_switch(register_t * p_regs, context_t * n_regs) {
    // Save registers of current running process to memory(which is current_process->regs)
    if(last_process) {
        last_process->regs.eax = p_regs->eax;
        last_process->regs.ebx = p_regs->ebx;
        last_process->regs.ecx = p_regs->ecx;
        last_process->regs.edx = p_regs->edx;
        last_process->regs.esi = p_regs->esi;
        last_process->regs.edi = p_regs->edi;
        last_process->regs.ebp = p_regs->ebp;
        last_process->regs.esp = p_regs->useresp;
        last_process->regs.eflags = p_regs->eflags;
        last_process->regs.eip = p_regs->eip;
        asm volatile("mov %%cr3, %0" : "=r"(last_process->regs.cr3));
    }


    // Switch page directory
    if(((page_directory_t*)n_regs->cr3) != NULL) {
        //uint32_t t = virtual2phys(kpage_dir, kpage_dir);
        //switch_page_directory(t, 1);
        switch_page_directory((page_directory_t*)n_regs->cr3, 1);
    }
    // Load regs(memory) to the real registers
    if(current_process->state == TASK_CREATED || current_process->state == TASK_LOADING)
        kernel_regs_switch(n_regs);
    else if(current_process->state == TASK_RUNNING)
        user_regs_switch(n_regs);
}


/*
 * This function is registered to the timer wakeup list, so it will be wakeup every 2/18 seconds
 * */
void schedule() {
    printf("Process Scheduler running\n");
    pcb_t * next;
    if(!list_size(process_list)) return;

    if(!current_process) {
        // First process, this will only happen when we create the user entry process, we'll make sure this first process never exits
        prev_jiffies = jiffies;
        current_process = list_peek_front(process_list);
        last_process = NULL;
        context_switch(NULL, &current_process->regs);
    }

    // save next process's listnode
    listnode_t * nextnode = (current_process->self)->next;
    if(current_process->state == TASK_ZOMBIE) {
        // Make sure we won't choose a zombie process to be run next
        list_remove_node(process_list, current_process->self);
        last_process = NULL;
    }

    // choose next process
    if(!nextnode)
        next = list_peek_front(process_list);
    else
        next = nextnode->val;

    current_process = next;
    if(current_process == NULL)
        PANIC("no process left, did you exit all user process ??? Never exit the userspace init process!!!!");
    context_switch(saved_context, &next->regs);
}


/*
 * Create a new process, load a program from filesystem and run it
 * */
void create_process(char * filename) {
    // Create and insert a process, the pcb struct is in kernel space
    pcb_t * p1 = kcalloc(sizeof(pcb_t), 1);
    p1->pid = allocate_pid();
    p1->regs.eip = (uint32_t)do_elf_load;
    p1->regs.eflags = 0x206; // enable interrupt
    p1->self = list_insert_front(process_list, p1);
    strcpy(p1->filename, filename);

    // 4kb initial stack
    p1->stack = (void*)0xC0000000;
    p1->regs.esp = (0xC0000000 - 4 * 1024);

    // Create an address space for the process, how ?
    // kmalloc a page directory for the process, then copy the entire kernel page dirs and tables(the frames don't have to be copied though)
    p1->page_dir = (void*)kmalloc_a(sizeof(page_directory_t));
    memset(p1->page_dir, 0, sizeof(page_directory_t));
    copy_page_directory(p1->page_dir, kpage_dir);
    p1->regs.cr3 = (uint32_t)virtual2phys(kpage_dir, p1->page_dir);
    p1->state = TASK_CREATED;

    // Now, the process has its own address space, stack,
    // schedule
    asm volatile("mov $1, %eax");
    asm volatile("int $0x80");
}
/*
 * Init process scheduler
 * */
void process_init() {
    process_list = list_create();
    // Tell the timer to call our process_scheduler every 2/18 seconds
    register_wakeup_call(schedule, 30.0/hz);
}
