#ifndef SYSCALL_H
#define SYSCALL_H
#include <system.h>
#include <isr.h>
#include <vfs.h>
#include <process.h>
#include <serial.h>

#define NUM_SYSCALLS 5

extern void * syscall_table[NUM_SYSCALLS];

void syscall_dispatcher(register_t * regs);

void syscall_init();

void _exit();


#endif
