#include <syscall.h>

/*
 * Syscall , exit current process
 * */
void _exit() {
    // First set the state of current process to zombie
    current_process->state = TASK_ZOMBIE;
    // maybe reclaim all the physical memory in-use by this process too(like call the paging system to reclaim all user page of current page directories), but we'll worry about this
    // laster, let's just waste those physical memory for now(and other possible resources possess by the process).

    // Then do schedule() so that scheduler transfer control to next process (when scheduler notice that it's a zombie process)
    // the timer handler will push a pointer to current process context to the schedule function
    schedule();
}
