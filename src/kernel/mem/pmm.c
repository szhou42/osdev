#include <pmm.h>
#include <system.h>
#include <string.h>
#include <printf.h>

uint8_t * bitmap = (uint8_t*)(&end);
uint8_t * mem_start;
uint32_t total_blocks;
uint32_t bitmap_size;

/*
 * Physical memory manager initialization, memset the memory bitmap to all 0
 * */
void pmm_init(uint32_t mem_size) {
    total_blocks = mem_size / BLOCK_SIZE;
    // For the given memory size, how many bytes is needed for the bitmap? (mem_size nees to be multiple of BLOCK_SIZE = 4096)
    bitmap_size = total_blocks / BLOCKS_PER_BUCKET;
    if(bitmap_size * BLOCKS_PER_BUCKET < total_blocks)
        bitmap_size++;

    // Clear bitmap
    memset(bitmap, 0, bitmap_size);

    // Start of all blcoks
    mem_start = (uint8_t*)BLOCK_ALIGN(((uint32_t)(bitmap + bitmap_size)));
#if 0
    printf("mem size:     %u mb\n", mem_size / (1024 * 1024));
    printf("total_blocks: %u\n", total_blocks);
    printf("bitmap addr:  0x%p\n", bitmap);
    printf("bitmap_size:  %u\n", bitmap_size);
    printf("mem_start:    0x%p\n", mem_start);

    for(int i = 0; i < bitmap_size; i++) {
        if(bitmap[i] != 0) {
            printf("bitmap is not all empty, fix it!\n");
        }
    }
#endif
}

/*
 * Find the first free block and return memory address of the block, also set corresponding bit in bitmap
 * */

uint32_t allocate_block() {
    uint32_t free_block = first_free_block();
    SETBIT(free_block);
    return free_block;
}

void free_block(uint32_t blk_num) {
    CLEARBIT(blk_num);
}

/*
 * Helper function for allocate_block(), return the first free block
 * */
uint32_t first_free_block() {
    uint32_t i;
    for(i = 0; i < total_blocks; i++) {
        if(!ISSET(i))
            return i;
    }
    printf("pmm: Running out of free blocks!\n");
    return (uint32_t) -1;
}

void simple_test() {

    printf("\n");
    uint32_t t1 = first_free_block();
    printf("first free block is %u\n", t1);

    void * ret = (void*)allocate_block();
    printf("first allocated block addr 0x%p\n", ret);


    uint32_t t2 = first_free_block();
    printf("second free block is %u\n", t2);

    ret = (void*)allocate_block();
    printf("second allocated block addr 0x%p\n", ret);

    free_block(t2);
    t2 = first_free_block();
    printf("third free block(after freeing the second) is %u\n", t2);

    free_block(t1);
    t1 = first_free_block();
    printf("third free block(after freeing the first) is %u\n", t1);
}
