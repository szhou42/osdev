#ifndef PAGING_H
#define PAGING_H
#include <system.h>
#include <isr.h>

#define PAGE_SIZE 4096

// Alignment related macro
#define IS_ALIGN(addr) ((((uint32_t)(addr)) | 0xFFFFF000) == 0)
#define PAGE_ALIGN(addr) ((((uint32_t)(addr)) & 0xFFFFF000) + 0x1000)

// Defone some address calculation macro
#define PAGEDIR_INDEX(vaddr) (((uint32_t)vaddr) >> 22)
#define PAGETBL_INDEX(vaddr) ((((uint32_t)vaddr) >>12) & 0x3ff)
#define PAGEFRAME_INDEX(vaddr) (((uint32_t)vaddr) & 0xfff)

// Paging register manipulation macro
#define SET_PGBIT(cr0) (cr0 = cr0 | 0x80000000)
#define CLEAR_PSEBIT(cr4) (cr4 = cr4 & 0xffffffef)

// Err code interpretation
#define ERR_PRESENT     0x1
#define ERR_RW          0x2
#define ERR_USER        0x4
#define ERR_RESERVED    0x8
#define ERR_INST        0x10


typedef struct page_dir_entry {
    unsigned int present    : 1;
    unsigned int rw         : 1;
    unsigned int user       : 1;
    unsigned int w_through  : 1;
    unsigned int cache      : 1;
    unsigned int access     : 1;
    unsigned int reserved   : 1;
    unsigned int page_size  : 1;
    unsigned int global     : 1;
    unsigned int available  : 3;
    unsigned int frame      : 20;
}page_dir_entry_t;

typedef struct page_table_entry {
    unsigned int present    : 1;
    unsigned int rw         : 1;
    unsigned int user       : 1;
    unsigned int reserved   : 2;
    unsigned int accessed   : 1;
    unsigned int dirty      : 1;
    unsigned int reserved2  : 2;
    unsigned int available  : 3;
    unsigned int frame      : 20;
}page_table_entry_t;


typedef struct page_table
{
    page_table_entry_t pages[1024];
} page_table_t;

typedef struct page_directory
{
    // The actual page directory entries(note that the frame number it stores is physical address)
    page_dir_entry_t tables[1024];
    // We need a table that contains virtual address, so that we can actually get to the tables
    page_table_t * ref_tables[1024];
} page_directory_t;

// Defined in entry.asm
extern page_directory_t * TEMP_PAGE_DIRECTORY;

// Defined in pmm.c
extern uint8_t * bitmap;
extern uint32_t  bitmap_size;

// Defined in paging.c
extern page_directory_t * kpage_dir;


void * virtual2phys(page_directory_t * dir, void * virtual_addr);

void * dumb_kmalloc(uint32_t size, int align);

void allocate_region(page_directory_t * dir, uint32_t start_va, uint32_t end_va, int iden_map, int is_kernel, int is_writable);

void allocate_page(page_directory_t * dir, uint32_t virtual_addr, uint32_t frame, int is_kernel, int is_writable);

void free_region(page_directory_t * dir, uint32_t start_va, uint32_t end_va, int free);

void free_page(page_directory_t * dir, uint32_t virtual_addr, int free);

void paging_init();

void switch_page_directory(page_directory_t * page_dir, uint32_t phys);

void enable_paging();

void * ksbrk(int size);

void copy_page_directory(page_directory_t * dst, page_directory_t * src);

page_table_t * copy_page_table(page_directory_t * src_page_dir, page_directory_t * dst_page_dir, uint32_t page_dir_idx, page_table_t * src);

void page_fault_handler(register_t * reg);
#endif
