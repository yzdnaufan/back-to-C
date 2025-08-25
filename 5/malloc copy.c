#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h> 
#include <unistd.h> 
#include <string.h>

#define PAGE_SIZE 4096
#define BLOCK_SIZE 16

typedef struct MyPageHeader{
    size_t size;
    size_t free_mem;
    struct MyPageHeader* next;
}MyPageHeader;

typedef struct MyBlockHeader{
    size_t size;
    bool is_free;
    struct MyBlockHeader* next;
}MyBlockHeader;

static MyPageHeader* first_page = NULL;

// Helper function to implement strcmp since it's used in main
int my_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

MyBlockHeader* find_free_block(size_t size) {
    MyPageHeader* current_page = first_page;
    
    while (current_page != NULL) {
        MyBlockHeader* current_block = (MyBlockHeader*)((char*)current_page + sizeof(MyPageHeader));
        
        // NOTEs: I dunno if this is correct or not aka. redundant maybe?
        // // Check if there are any blocks in this page
        // if (current_page->free_mem == current_page->size - sizeof(MyPageHeader)) {
        //     // No blocks created yet, skip to next page
        //     current_page = current_page->next;
        //     continue;
        // }
        
        //cycle through blocks in the page, and make sure we don't go out of page bounds,
        //if block is out of page bounds, stop the loop and go to next page
        while (current_block != NULL && 
               (char*)current_block < (char*)current_page + current_page->size) {
            if (current_block->is_free && current_block->size >= size) {
                return current_block;
            }
            current_block = current_block->next;
        }
        current_page = current_page->next;
    }
    return NULL;
}

MyBlockHeader* create_new_block(size_t size, MyPageHeader* page) {
    if (page->free_mem < size + sizeof(MyBlockHeader)) {
        return NULL;
    }
    
    // Find the end of existing blocks or start of page if no blocks exist
    MyBlockHeader* current_block = (MyBlockHeader*)((char*)page + sizeof(MyPageHeader));
    MyBlockHeader* last_block = NULL;
    
    // Find the last block or determine if this is the first block
    while (current_block != NULL) {
        //check page boundary
        if(current_block>= (char*)page + page->size){
            break;
        }
        last_block = current_block;
        current_block = current_block->next;
    }
    
    // Calculate position for new block
    char* new_block_pos;
    if (last_block == NULL) {
        // First block in the page
        new_block_pos = (char*)page + sizeof(MyPageHeader);
    } else {
        // After the last block
        new_block_pos = (char*)last_block + sizeof(MyBlockHeader) + last_block->size;
    }
    
    MyBlockHeader* new_block = (MyBlockHeader*)new_block_pos;
    new_block->size = size;
    new_block->is_free = false;  // Mark as allocated
    new_block->next = NULL;
    
    // Link the previous block to this new block
    if (last_block != NULL) {
        last_block->next = new_block;
    }
    
    // Update page free memory
    page->free_mem -= (size + sizeof(MyBlockHeader));
    
    return new_block;
}

MyPageHeader* create_new_page(size_t size) {
    size_t needed_size = sizeof(MyPageHeader) + sizeof(MyBlockHeader) + size;
    size_t pages_needed = (needed_size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t total_size = pages_needed * PAGE_SIZE;
    
    void* new_mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_mem == MAP_FAILED) {
        return NULL;
    }
    
    MyPageHeader* new_page_header = (MyPageHeader*)new_mem;
    new_page_header->size = total_size;
    new_page_header->free_mem = total_size - sizeof(MyPageHeader);
    new_page_header->next = NULL;
    
    // Link to existing pages
    if (first_page == NULL) {
        first_page = new_page_header;
    } else {
        MyPageHeader* current = first_page;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_page_header;
    }
    
    return new_page_header;
}

void* my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Align size to minimum block size
    if (size < BLOCK_SIZE) {
        size = BLOCK_SIZE;
    }
    
    // Try to find a free block first
    MyBlockHeader* block = find_free_block(size);
    
    if (block != NULL) {
        block->is_free = false;
        return (void*)((char*)block + sizeof(MyBlockHeader));
    }
    
    // No suitable free block found, need a new page
    MyPageHeader* page = NULL;
    
    // Check if we can use an existing page
    if (first_page != NULL) {
        MyPageHeader* current_page = first_page;
        while (current_page != NULL) {
            if (current_page->free_mem >= size + sizeof(MyBlockHeader)) {
                page = current_page;
                break;
            }
            current_page = current_page->next;
        }
    }
    
    // Create new page if needed
    if (page == NULL) {
        page = create_new_page(size);
        if (page == NULL) {
            return NULL; // Out of memory
        }
    }
    
    // Create new block in the page
    block = create_new_block(size, page);
    if (block == NULL) {
        return NULL;
    }
    
    return (void*)((char*)block + sizeof(MyBlockHeader));
}

// Helper function to coalesce adjacent free blocks
void coalesce_blocks(MyPageHeader* page) {
    MyBlockHeader* current = (MyBlockHeader*)((char*)page + sizeof(MyPageHeader));
    
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            // Merge current block with next block
            MyBlockHeader* next_block = current->next;
            current->size += sizeof(MyBlockHeader) + next_block->size;
            current->next = next_block->next;
            
            // Update page free memory (we're removing one block header)
            page->free_mem += sizeof(MyBlockHeader);
            
            // Continue checking from current block (don't advance)
            continue;
        }
        current = current->next;
    }
}

// Helper function to check if a page is completely free
bool is_page_empty(MyPageHeader* page) {
    MyBlockHeader* current = (MyBlockHeader*)((char*)page + sizeof(MyPageHeader));
    
    // If free memory equals total memory minus page header, page is empty
    if (page->free_mem == page->size - sizeof(MyPageHeader)) {
        return true;
    }
    
    // Check if all blocks are free
    while (current != NULL && 
           (char*)current < (char*)page + page->size - page->free_mem) {
        if (!current->is_free) {
            return false;
        }
        current = current->next;
    }
    return true;
}

// Helper function to remove an empty page from the page list
void remove_empty_page(MyPageHeader* page_to_remove) {
    if (first_page == page_to_remove) {
        // Removing the first page
        first_page = page_to_remove->next;
    } else {
        // Find the previous page
        MyPageHeader* prev = first_page;
        while (prev != NULL && prev->next != page_to_remove) {
            prev = prev->next;
        }
        if (prev != NULL) {
            prev->next = page_to_remove->next;
        }
    }
    
    // Unmap the page from memory
    munmap(page_to_remove, page_to_remove->size);
}

void my_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    // Find the block header
    MyBlockHeader* block = (MyBlockHeader*)((char*)ptr - sizeof(MyBlockHeader));
    
    // Validate that this is a valid allocated block
    if (block->is_free) {
        printf("Warning: Attempting to free already freed memory at %p\n", ptr);
        return;
    }
    
    // Find which page this block belongs to
    MyPageHeader* current_page = first_page;
    MyPageHeader* block_page = NULL;
    
    while (current_page != NULL) {
        char* page_start = (char*)current_page;
        char* page_end = page_start + current_page->size;
        
        if ((char*)block >= page_start && (char*)block < page_end) {
            block_page = current_page;
            break;
        }
        current_page = current_page->next;
    }
    
    if (block_page == NULL) {
        printf("Error: Could not find page for block at %p\n", ptr);
        return;
    }
    
    printf("Freeing %zu bytes at %p (block header at %p)\n", block->size, ptr, (void*)block);
    
    // Mark block as free (DON'T update free_mem counter here - it's misleading)
    // The free_mem represents unallocated space, not freed blocks
    block->is_free = true;
    
    // Coalesce adjacent free blocks to reduce fragmentation
    coalesce_blocks(block_page);
    
    // Check if the entire page is now free and can be returned to system
    if (is_page_empty(block_page) && first_page != NULL && first_page->next != NULL) {
        // Only remove page if it's not the last page (keep at least one page)
        printf("Page is completely free, returning %zu bytes to system\n", block_page->size);
        remove_empty_page(block_page);
    }
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
    
    printf("Allocating 128 bytes...\n");
    void* ptr2 = my_malloc(128);
    printf("Allocated ptr2: %p\n", ptr2);
    print_memory_usage();
    
    printf("Allocating 32 bytes...\n");
    void* ptr3 = my_malloc(32);
    printf("Allocated ptr3: %p\n", ptr3);
    print_memory_usage();
    
    printf("Freeing ptr2 (128 bytes)...\n");
    my_free(ptr2);
    print_memory_usage();
    
    printf("Freeing ptr1 (64 bytes) - should coalesce with ptr2...\n");
    my_free(ptr1);
    print_memory_usage();
    
    printf("Allocating 100 bytes (should reuse coalesced space)...\n");
    void* ptr4 = my_malloc(100);
    printf("Allocated ptr4: %p\n", ptr4);
    print_memory_usage();
    
    printf("Allocating large block (5000 bytes - will need new page)...\n");
    void* ptr5 = my_malloc(5000);
    printf("Allocated ptr5: %p\n", ptr5);
    print_memory_usage();
    
    printf("Freeing ptr5 (large block)...\n");
    my_free(ptr5);
    print_memory_usage();
    
    printf("Freeing remaining blocks...\n");
    my_free(ptr3);
    my_free(ptr4);
    print_memory_usage();
    
    // Test double free detection
    printf("Testing double free detection...\n");
    my_free(ptr1); // Should show warning
    
    // Test string comparison
    printf("\n=== Testing String Comparison ===\n");
    char s_1[] = "halo";
    char s_2[] = "hala";

    int res = my_strcmp(s_1, s_2);
    if (res > 0) {
        printf("String '%s' comes after '%s' lexically\n", s_1, s_2);
    } else if (res == 0) {
        printf("Both strings are the same\n");
    } else {
        printf("String '%s' comes before '%s' lexically\n", s_1, s_2);
    }
    
    return 0;
}