#ifndef THREAD_H
#define THREAD_H
#include <system.h>
#include <process.h>
#include <list.h>

// Thread control block
typedef struct tcb {
    listnode_t * self;
    pcb_t * parent;
    void * stack;
    int state;
    context_t regs;
}tcb_t;

#endif

