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
    // struct MyPageHeader* prev;
}MyPageHeader;

typedef struct MyBlockHeader{
    size_t size;
    bool is_free;
    struct MyBlockHeader* next;
    // struct MyBlockHeader* prev;
}MyBlockHeader;

static MyPageHeader* first_page = NULL;

MyBlockHeader* find_free_block(size_t size){
    MyPageHeader* current_page = first_page;
    //if there is a page, cycle through it then check for first block whether it is free and has enough size
    while(current_page != NULL){
        //find the first block in the page
        MyBlockHeader* current_block = (MyBlockHeader*)((char*)current_page + sizeof(MyPageHeader));
        
        //cycle to available block in the page, and make sure we don't go out of page bounds,
        //if block is out of page bounds, stop the loop and go to next page
        while(current_block != NULL && (char*)current_block < (char*)current_page + current_page->size){
            if(current_block->is_free && current_block->size >= size && current_block != NULL){
                return current_block;
            }
                current_block = current_block->next;
            }
        current_page = current_page->next;
    }
    return NULL;
}

MyBlockHeader* create_new_block(size_t size, MyPageHeader* page){
    //TODO: iterate after page header adress to find suitable block
    if(page->free_mem >= size + sizeof(MyBlockHeader)){
        MyBlockHeader* current_block = (MyBlockHeader*)((char*)page + sizeof(MyPageHeader));
        while(current_block!=NULL){
            if(current_block->is_free == false & current_block->next==NULL){
                MyBlockHeader* new_block_header = (MyBlockHeader*) ((char*)current_block + sizeof(MyPageHeader)+current_block->size);
                new_block_header->size = size;
                new_block_header->is_free = true;
                new_block_header->next = NULL;
                current_block->next = new_block_header;
            }
            current_block = current_block->next;
        }

    }
    else{
        return NULL;
    }
    return NULL;
}

MyPageHeader* create_new_page(size_t size){
    
    size_t needed_size = sizeof(MyPageHeader) + size;
    size_t pages_needed = (needed_size+PAGE_SIZE-1)/ PAGE_SIZE;
    size_t pages_size = pages_needed * PAGE_SIZE;
    
    void* new_mem = mmap(NULL, pages_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_mem == MAP_FAILED) {
        return NULL; // Out of memory
    }
    
    MyPageHeader* new_page_header = (MyPageHeader*) new_mem;
    new_page_header->size = pages_size;
    new_page_header->free_mem = pages_size - needed_size + size;
    new_page_header->next = NULL;
    
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
    MyPageHeader* current_page = first_page;
    if(current_page != NULL){
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

int main() {
    // Write C code here

    //TODO: test the malloc functions
    printf("Hello world");


    return 0;
}