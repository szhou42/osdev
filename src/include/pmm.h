#ifndef PMM_H
#define PMM_H
#include <system.h>

#define BLOCK_SIZE 4096
#define BLOCKS_PER_BUCKET 8
// Define some bit manipulating operations
#define SETBIT(i) bitmap[i / BLOCKS_PER_BUCKET] = bitmap[i / BLOCKS_PER_BUCKET] | (1 << (i % BLOCKS_PER_BUCKET))
#define CLEARBIT(i) bitmap[i / BLOCKS_PER_BUCKET] = bitmap[i / BLOCKS_PER_BUCKET] & (~(1 << (i % BLOCKS_PER_BUCKET)))
#define ISSET(i) ((bitmap[i / BLOCKS_PER_BUCKET] >> (i % BLOCKS_PER_BUCKET)) & 0x1)
#define GET_BUCKET32(i) (*((uint32_t*) &bitmap[i / 32]))

#define BLOCK_ALIGN(addr) (((addr) & 0xFFFFF000) + 0x1000)

// Defined in link.ld, indicate the end of kernel code/data
extern uint32_t end;

void pmm_init(uint32_t mem_size);

uint32_t allocate_block();

void free_block(uint32_t blk_num);

uint32_t first_free_block();

void simple_test();
#endif
