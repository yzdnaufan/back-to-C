// Online C compiler to run C program online
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h> 
#include <unistd.h> 

#define PAGE_SIZE 4096
#define BLOCK_SIZE 16

typedef struct MyPageHeader{
    size_t size;
    size_t free_mem;
    struct MyPageHeader* next;
    struct MyPageHeader* prev;
}MyPageHeader;

typedef struct MyBlockHeader{
    size_t size;
    bool is_free;
    struct MyBlockHeader* next;
    struct MyBlockHeader* prev;
}MyBlockHeader;


static MyPageHeader* first_page = NULL;

static int debug_counter = 0;

MyBlockHeader* find_free_block(size_t size){
    MyPageHeader* current_page = first_page;
    // Iterate through each page
    while(current_page != NULL){
        // Only search for blocks if the page actually has any allocated blocks.
        // We know blocks exist if the free memory is less than the total available block space.
        if (current_page->free_mem < current_page->size - sizeof(MyPageHeader)) {
            MyBlockHeader* current_block = (MyBlockHeader*)((char*)current_page + sizeof(MyPageHeader));
            
            // Traverse the linked list of blocks on this page
            while(current_block != NULL){
                // A safety check to prevent running off the page in case of a corrupted list
                if ((char*)current_block >= (char*)current_page + current_page->size) {
                    break; 
                }

                if(current_block->is_free && current_block->size >= size){
                    return current_block;
                }
                current_block = current_block->next;
            }
        }
        current_page = current_page->next;
    }
    return NULL; // No suitable block found
}


MyBlockHeader* create_new_block(size_t size, MyPageHeader* page){
   // Check if the page has enough free memory for our block
   if(page->free_mem < size + sizeof(MyBlockHeader)){
        return NULL;
   }

    // The new block will be placed at the beginning of the free space.
    // This is calculated by taking the page's start address and adding the amount of memory already in use.
    char* new_block_pos = (char*)page + (page->size - page->free_mem);
    MyBlockHeader* new_block = (MyBlockHeader*) new_block_pos;
    
    MyBlockHeader* last_block = NULL;

    // Check if this is NOT the first block in the page.
    // If it is the first block, last_block remains NULL.
    if (page->free_mem < page->size - sizeof(MyPageHeader)) {
        // If we are here, blocks already exist. Find the last one by traversing the list.
        last_block = (MyBlockHeader*)((char*)page + sizeof(MyPageHeader));
        while (last_block->next != NULL) {
            last_block = last_block->next;
        }
    }

    // Set up the new block's properties
    new_block->is_free = false;
    new_block->size = size;
    new_block->next = NULL;
    new_block->prev = last_block;

    // Link the previous block (if it exists) to the new one
    if (last_block != NULL) {
        last_block->next = new_block;
    }
    
    // Update the page's free memory counter
    page->free_mem -= (size + sizeof(MyBlockHeader));

    return new_block;
}

MyPageHeader* create_new_page(size_t size){
    
    //calculate how many pages needed
    size_t needed_size = sizeof(MyPageHeader) + size;
    size_t pages_needed = (needed_size+PAGE_SIZE-1)/ PAGE_SIZE;
    size_t pages_size = pages_needed * PAGE_SIZE;
    
    //allocate memory using mmap
    void* new_mem = mmap(NULL, pages_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_mem == MAP_FAILED) {
        return NULL; // Out of memory
    }
    
    //create and initialize new page header
    MyPageHeader* new_page_header = (MyPageHeader*) new_mem;
    new_page_header->size = pages_size;
    new_page_header->free_mem = pages_size - needed_size + size;
    new_page_header->next = NULL;

    //linking to existing pages
    if(first_page == NULL){
        first_page = new_page_header;
        new_page_header->prev = NULL;
        new_page_header->next = NULL; 
    }
    else{
        MyPageHeader* current = first_page;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_page_header;
        new_page_header->prev = current;
        new_page_header->next = NULL;
    }
    
    return new_page_header;
}

void* my_malloc(size_t size){
    // set minimum block size
    if(size < BLOCK_SIZE){
        size = BLOCK_SIZE;
    }

    MyPageHeader* page = NULL;

    //cycle through available block to find free memory, if found, return the address
    MyBlockHeader* block_mem = find_free_block(size);
    if(block_mem != NULL){
        block_mem->is_free = false;
        return (void*)((char*)block_mem + sizeof(MyBlockHeader));
    }
    
    //if no free block found, try to find a page with enough memory
    if(first_page != NULL){
        MyPageHeader* current_page = first_page;
        while(current_page != NULL){
            if(current_page->free_mem >= size + sizeof(MyBlockHeader)){
                page = current_page;
                break;
            }
            current_page = current_page->next;
        }
    }

    //if no page with enough memory found, create a new page
    if(page == NULL){
        page = create_new_page(size);
        if(page == NULL){
            return NULL; // Out of memory
        }
    }

    //create a new block in the page
    block_mem = create_new_block(size, page);
    if(block_mem == NULL){
        return NULL;
    }

    return (void*)((char*)block_mem+sizeof(MyBlockHeader));
}

// Function to print detailed memory usage statistics
void print_memory_usage() {
    printf("\n=== Memory Usage Report ===\n");
    
    if (first_page == NULL) {
        printf("No memory allocated yet.\n");
        return;
    }
    
    size_t total_pages = 0;
    size_t total_system_memory = 0;
    size_t total_unallocated = 0;
    size_t total_user_data = 0;
    size_t total_overhead = 0;
    size_t total_blocks = 0;
    size_t free_blocks = 0;
    size_t used_blocks = 0;
    size_t freed_but_not_reused = 0;
    
    MyPageHeader* current_page = first_page;
    
    while (current_page != NULL) {
        total_pages++;
        total_system_memory += current_page->size;
        total_unallocated += current_page->free_mem;
        total_overhead += sizeof(MyPageHeader);
        
        printf("\nPage %zu:\n", total_pages);
        printf("  Total size: %zu bytes\n", current_page->size);
        printf("  Unallocated space: %zu bytes\n", current_page->free_mem);
        printf("  Allocated space: %zu bytes\n", current_page->size - current_page->free_mem);
        
        // Traverse blocks in this page
        MyBlockHeader* current_block = (MyBlockHeader*)((char*)current_page + sizeof(MyPageHeader));
        size_t block_count = 0;
        size_t page_user_data = 0;
        size_t page_freed_data = 0;
        size_t page_block_overhead = 0;
        
        printf("  Blocks in this page:\n");
        
        while (current_block != NULL && 
               (char*)current_block < (char*)current_page + current_page->size - current_page->free_mem) {
            block_count++;
            total_blocks++;
            page_block_overhead += sizeof(MyBlockHeader);
            
            printf("    Block %zu: %zu bytes, %s (header at %p, data at %p)\n", 
                   block_count, current_block->size, 
                   current_block->is_free ? "FREE" : "USED",
                   (void*)current_block,
                   (void*)((char*)current_block + sizeof(MyBlockHeader)));
            
            if (current_block->is_free) {
                free_blocks++;
                page_freed_data += current_block->size;
                freed_but_not_reused += current_block->size;
            } else {
                used_blocks++;
                page_user_data += current_block->size;
                total_user_data += current_block->size;
            }
            
            current_block = current_block->next;
        }
        
        total_overhead += page_block_overhead;
        
        printf("  Summary: %zu blocks\n", block_count);
        printf("    - User data in used blocks: %zu bytes\n", page_user_data);
        printf("    - Data in freed blocks: %zu bytes\n", page_freed_data);
        printf("    - Block headers overhead: %zu bytes\n", page_block_overhead);
        printf("    - Unallocated space: %zu bytes\n", current_page->free_mem);
        
        current_page = current_page->next;
    }
    
    printf("\n=== Overall Statistics ===\n");
    printf("Total pages: %zu\n", total_pages);
    printf("Total system memory: %zu bytes (%.2f KB)\n", 
           total_system_memory, total_system_memory / 1024.0);
    printf("  ├─ Active user data: %zu bytes (%.2f KB)\n", 
           total_user_data, total_user_data / 1024.0);
    printf("  ├─ Freed but not reused: %zu bytes (%.2f KB)\n", 
           freed_but_not_reused, freed_but_not_reused / 1024.0);
    printf("  ├─ Metadata overhead: %zu bytes (%.2f KB)\n", 
           total_overhead, total_overhead / 1024.0);
    printf("  └─ Unallocated space: %zu bytes (%.2f KB)\n", 
           total_unallocated, total_unallocated / 1024.0);
    printf("Total blocks: %zu (Used: %zu, Free: %zu)\n", 
           total_blocks, used_blocks, free_blocks);
    if (total_blocks > 0) {
        printf("Block utilization: %.2f%% (%zu/%zu blocks active)\n", 
               (double)(used_blocks * 100) / total_blocks, used_blocks, total_blocks);
    }
    if (total_system_memory > 0) {
        printf("Memory utilization: %.2f%% (%zu bytes active user data)\n", 
               (double)(total_user_data * 100) / total_system_memory, total_user_data);
        printf("Memory overhead: %.2f%% (%zu bytes metadata)\n", 
               (double)(total_overhead * 100) / total_system_memory, total_overhead);
    }
    printf("=============================\n\n");
}

int main() {
    printf("=== Testing Custom Memory Allocator ===\n\n");
    
    // Initial state
    print_memory_usage();
    
    // Test the memory allocator
    printf("Allocating 64 bytes...\n");
    void* ptr1 = my_malloc(64);
    printf("Allocated ptr1: %p\n", ptr1);
    print_memory_usage();
    
    // printf("Allocating 128 bytes...\n");
    // void* ptr2 = my_malloc(128);
    // printf("Allocated ptr2: %p\n", ptr2);
    // print_memory_usage();
    
    // printf("Allocating 32 bytes...\n");
    // void* ptr3 = my_malloc(32);
    // printf("Allocated ptr3: %p\n", ptr3);
    // print_memory_usage();
    
    // printf("Freeing ptr2 (128 bytes)...\n");
    // // my_free(ptr2);
    // print_memory_usage();
    
    // printf("Freeing ptr1 (64 bytes) - should coalesce with ptr2...\n");
    // // my_free(ptr1);
    // print_memory_usage();
    
    // printf("Allocating 100 bytes (should reuse coalesced space)...\n");
    // void* ptr4 = my_malloc(100);
    // printf("Allocated ptr4: %p\n", ptr4);
    // print_memory_usage();
    
    // printf("Allocating large block (5000 bytes - will need new page)...\n");
    // void* ptr5 = my_malloc(5000);
    // printf("Allocated ptr5: %p\n", ptr5);
    // print_memory_usage();
    
    // printf("Freeing ptr5 (large block)...\n");
    // // my_free(ptr5);
    // print_memory_usage();
    
    // printf("Freeing remaining blocks...\n");
    // // my_free(ptr3);
    // // my_free(ptr4);
    // print_memory_usage();
    
    // // Test double free detection
    // printf("Testing double free detection...\n");
    // // my_free(ptr1); // Should show warning
    
    
    return 0;
}