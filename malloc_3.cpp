#include <unistd.h>  
#include <cstddef>
#include <iostream>  
#include <cstring>
#include <sys/mman.h> // For mmap



const long int MAX_SIZE=100000000;
const long int MAX_ORDER=10;
const size_t MMAP_THRESHOLD = 128 * 1024;

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    MallocMetadata** buddies;// array to put the buddies in. each block can have a buddy in every order but the max. you fill the array one step from top to down every time you split them.

};

size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;
size_t num_meta_data_bytes = 0;

MallocMetadata* order_arr[MAX_ORDER + 1]= {nullptr};
MallocMetadata* mmap_list = nullptr; // Separate list for mmap blocks

bool initialized = false;

void buddy_allocate(){

    int num=32;
    void* ptr=sbrk(num*MMAP_THRESHOLD);

    if (ptr == (void*) -1)
        return;

    MallocMetadata *tail = NULL, *head=NULL;
    for(int i=0;i<num;i++){

        MallocMetadata* current = (MallocMetadata*)ptr;

        current->size = MMAP_THRESHOLD;
        current->is_free = true;
        current->next = NULL;
        current->prev = tail;

        if(tail){
            tail->next = current;
        }

        tail=current;

        if(head==NULL){
            head=current;
        }

        ptr = (char*)ptr + MMAP_THRESHOLD;  // Move to the next block
        num_allocated_blocks++;
        num_allocated_bytes+=MMAP_THRESHOLD;
        num_free_blocks++;
        num_free_bytes+=MMAP_THRESHOLD;
        num_meta_data_bytes+=sizeof(MallocMetadata);

    }

    order_arr[MAX_ORDER]=head;
    return;

}



// Helper funcation for challange 3,
// Splits a block until it matches the requested size
MallocMetadata* split_block(int order, size_t required_size) {
    while (order > 0) {
        MallocMetadata* block = order_arr[order];

        if (!block){
            return nullptr;
        }

        order_arr[order] = block->next;

        if (order_arr[order]) {
            order_arr[order]->prev = nullptr;
        }

        size_t new_size = block->size / 2;
        MallocMetadata* buddy = (MallocMetadata*)((char*)block + new_size);
        buddy->size = new_size;
        buddy->is_free = true;
        buddy->next = order_arr[order - 1];

        if (order_arr[order - 1]) {
            order_arr[order - 1]->prev = buddy;
        }
        order_arr[order - 1] = buddy;
        block->size = new_size;

        order--;
    }
    return order_arr[order];
}


void* smalloc(size_t size){
    
    if(!initialized){
        buddy_allocate();
        initialized=true;
    }

    if(size==0||size>MAX_SIZE){
        return NULL;
    }

    //Handle Challange 3
    if (size >= MMAP_THRESHOLD) { 
        void* ptr = mmap(nullptr, size + sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        
        if (ptr == MAP_FAILED){
            return nullptr;
        }

        MallocMetadata* metadata = (MallocMetadata*)ptr;
        metadata->size = size;
        metadata->is_free = false;
        metadata->next = mmap_list;
        mmap_list = metadata;

        num_allocated_blocks++;
        num_allocated_bytes += size;

        return (void*)(metadata + 1);
    }

    int order_size=64;
    int order;

    for (order = 0; order <= MAX_ORDER; order++) {
        order_size *= 2;
        if (size <= order_size) break;
    }

    MallocMetadata* block = order_arr[order];
    if (!block) {
        block = split_block(order, size);
        if (!block){
            return nullptr;
        }
    }

    order_arr[order] = block->next;
    block->is_free = false;
    num_free_blocks--;
    num_free_bytes -= block->size;

    return (void*)(block + 1);
}

void* scalloc(size_t num, size_t size){
    size_t total_size = num * size;
    
    void* ptr = smalloc(total_size);
    if (!ptr){
        return NULL;
    }

    std::memset(ptr, 0, total_size);
    return ptr;
}

void sfree(void* p){
    if(!p){
        return;
    }

    MallocMetadata* metadata = (MallocMetadata*)p - 1;

    if (metadata->is_free){
        return;
    }

    metadata->is_free = true;
    num_free_blocks++;
    num_free_bytes += metadata->size;

    int order = 0;
    size_t block_size = metadata->size;
    while (block_size > 64) {
        block_size /= 2;
        order++;
    }

    while (order < MAX_ORDER) {
        size_t buddy_offset = ((char*)metadata - (char*)sbrk(0)) ^ metadata->size;
        MallocMetadata* buddy = (MallocMetadata*)((char*)sbrk(0) + buddy_offset);
        if (!buddy->is_free || buddy->size != metadata->size){
            break;
        }
        if (buddy->prev){
            buddy->prev->next = buddy->next;
        }
        if (buddy->next){
            buddy->next->prev = buddy->prev;
        }
        if (metadata > buddy){
            metadata = buddy;
        }
        metadata->size *= 2;
        order++;
    }

    metadata->next = order_arr[order];
    order_arr[order] = metadata;
}

void* srealloc(void* oldp, size_t size){
    if(!oldp){
        return smalloc(size);
    }
    MallocMetadata* metadata = (MallocMetadata*)oldp - 1;
    if(size<=metadata->size){
        return oldp;
    }
    void* ptr= smalloc(size);
    if(ptr){
        std::memmove(ptr, oldp, metadata->size);
        sfree(oldp);
        return ptr;
    }
    return NULL;

}

size_t _num_free_blocks() { return num_free_blocks; }

size_t _num_free_bytes() { return num_free_bytes; }

size_t _num_allocated_blocks() { return num_allocated_blocks; }

size_t _num_allocated_bytes() { return num_allocated_bytes; }

size_t _num_meta_data_bytes() { return num_meta_data_bytes; }

size_t _size_meta_data() { return sizeof(MallocMetadata); }
