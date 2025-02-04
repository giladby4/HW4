#include <unistd.h>  
#include <cstddef>
#include <iostream>  
#include <cstring>
#include <sys/mman.h> // For mmap



const long int MAX_SIZE=100000000;
const long int MAX_ORDER=10;
const size_t MMAP_THRESHOLD = 128 * 1024;
void* buddy_base = nullptr;


struct MallocMetadata {
    size_t size;
    bool is_free;
    bool is_mmap_alloc; // Flag to distinguish mmap blocks
    MallocMetadata* next;
    MallocMetadata* prev;
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

    const int num = 32;
    void* ptr=sbrk(num*MMAP_THRESHOLD);

    if (ptr == (void*) -1){
        return;
    }

    buddy_base = ptr;

    MallocMetadata* head = nullptr;
    MallocMetadata* tail = nullptr;

    for(int i=0;i<num;i++){ 

        MallocMetadata* current = (MallocMetadata*)ptr;

        current->size = MMAP_THRESHOLD;
        current->is_free = true;
        current->next = nullptr;
        current->prev = tail;
        current->is_mmap_alloc = false;


        if(tail){
            tail->next = current;
        }

        tail=current;

        if (!head){
            head=current;
        }

        num_allocated_blocks++;
        num_allocated_bytes += (MMAP_THRESHOLD - sizeof(MallocMetadata));
        num_free_blocks++;
        num_free_bytes += (MMAP_THRESHOLD - sizeof(MallocMetadata));
        num_meta_data_bytes+=sizeof(MallocMetadata);

        ptr = (char*)ptr + MMAP_THRESHOLD;


    }

    order_arr[MAX_ORDER]=head;
}


// Given a block, compute its buddy address using XOR.
MallocMetadata* find_buddy(MallocMetadata* block) {
    uintptr_t base = (uintptr_t)buddy_base;
    uintptr_t offset = (uintptr_t)block - base;
    uintptr_t buddy_offset = offset ^ block->size;
    return (MallocMetadata*)(base + buddy_offset);
}



// Helper funcation for challange 3,
// Splitting: given a block from order 'curr_order', split until we reach 'target_order'.
MallocMetadata* split_block(int curr_order, int target_order) {
    MallocMetadata* block = order_arr[curr_order];
    if (!block){
        return nullptr;
    }
    // Remove block from free list:
    order_arr[curr_order] = block->next;
    if (order_arr[curr_order]){
        order_arr[curr_order]->prev = nullptr;
    }
    num_free_blocks--;
    num_free_bytes -= (block->size - sizeof(MallocMetadata));
    
    while (curr_order > target_order) {
        curr_order--;
        size_t new_size = block->size / 2;
        
        // Create buddy block:
        MallocMetadata* buddy = (MallocMetadata*)((char*)block + new_size);
        buddy->size = new_size;
        buddy->is_free = true;
        buddy->is_mmap_alloc = false;

        buddy->next = order_arr[curr_order];
        buddy->prev = nullptr;
        if (order_arr[curr_order]){
            order_arr[curr_order]->prev = buddy;
        }
        order_arr[curr_order] = buddy;
        
        num_allocated_blocks++;  // new block created
        num_allocated_bytes -= sizeof(MallocMetadata);
        num_meta_data_bytes += sizeof(MallocMetadata);
        num_free_blocks++;

        num_free_bytes += (new_size - sizeof(MallocMetadata));
        
        block->size = new_size;
    }
    return block;
}


void* smalloc(size_t size) {
    if (!initialized) {
        buddy_allocate();
        initialized = true;
    }
    
    if (size == 0 || size > MAX_SIZE)
        return nullptr;
    
    // Mmap branch for large allocations.
    if (size >= MMAP_THRESHOLD) {
        void* ptr = mmap(nullptr, size + sizeof(MallocMetadata),
                         PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        if (ptr == MAP_FAILED){
            return nullptr;
        }

        MallocMetadata* metadata = (MallocMetadata*)ptr;
        metadata->size = size;
        metadata->is_free = false;
        metadata->is_mmap_alloc = true;


        // Insert into mmap_list.
        metadata->next = mmap_list;
        mmap_list = metadata;
        
        num_allocated_blocks++;
        num_allocated_bytes += size;
        num_meta_data_bytes += sizeof(MallocMetadata);
        return (void*)(metadata + 1);
    }
    
    // For buddy blocks, determine the required block size.
    size_t needed = size + sizeof(MallocMetadata);
    int target_order = 0;
    size_t block_size = 128;

    while (block_size < needed && target_order <= MAX_ORDER) {
        block_size *= 2;
        target_order++;
    }
    if (target_order > MAX_ORDER)
        return nullptr; // request too large for buddy system
    
    // Find the smallest available block that fits.
    int curr_order = target_order;
    while (curr_order <= MAX_ORDER && order_arr[curr_order] == nullptr){
        curr_order++;
    }

    if (curr_order > MAX_ORDER)
        return nullptr;
    
    // Split down to target_order if necessary.
    MallocMetadata* block = split_block(curr_order, target_order);
    if (!block){
        return nullptr;
    }
    
    block->is_free = false;
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

    MallocMetadata* block = (MallocMetadata*)p - 1;

    // Check if the block is already free; if so, do nothing.
    if (block->is_free) {
        return;
    }

    // If the block is an mmap allocation.
    if (block->is_mmap_alloc) {
        munmap(block, block->size + sizeof(MallocMetadata));
        num_allocated_blocks--;
        num_allocated_bytes -= block->size;
        return;
    }

    block->is_free = true;
    num_free_blocks++;
    num_free_bytes += (block->size - sizeof(MallocMetadata));

    // Determine the order of the block.
    int order = 0;
    size_t tmp = block->size;
    while (tmp > 128) { 
        tmp /= 2;
        order++;
    }

    // Try to merge with buddy blocks.
    while (order < MAX_ORDER) {
        MallocMetadata* buddy = find_buddy(block);
        if (!buddy || !buddy->is_free || buddy->size != block->size){
            break;
        }
        // Remove buddy from its free list.
        if (buddy->prev){
            buddy->prev->next = buddy->next;
        }
        if (buddy->next){
            buddy->next->prev = buddy->prev;
        }
        if (order_arr[order] == buddy){
            order_arr[order] = buddy->next;
        }

        num_free_blocks--;
    
        num_meta_data_bytes -= sizeof(MallocMetadata);
        num_allocated_bytes += sizeof(MallocMetadata);
        num_allocated_blocks--;
        num_free_bytes += sizeof(MallocMetadata);
    
        if (buddy < block)
            block = buddy;
        block->size *= 2;
        order++;
    }   

    block->next = order_arr[order];
    block->prev = nullptr;
    if (order_arr[order]){
        order_arr[order]->prev = block;
    }
    order_arr[order] = block;
}

void* srealloc(void* oldp, size_t size) {
    // If oldp is NULL, behave like smalloc.
    if (!oldp) {
        return smalloc(size);
    }

    MallocMetadata* block = (MallocMetadata*)oldp - 1;

    // For mmap-allocated blocks, handle separately.
    if (block->is_mmap_alloc) {
        // mmap branch
        if (size == block->size){
            return oldp;
        }
        void* newp = smalloc(size);
        if (!newp){
            return nullptr;
        }
        std::memmove(newp, oldp, std::min(size, block->size));
        munmap(block, block->size + sizeof(MallocMetadata));
        num_allocated_blocks--;
        num_allocated_bytes -= block->size;
        return newp;
    }

    if (size <= block->size - sizeof(MallocMetadata)) {
        return oldp;
    }

    // Compute the order of the current block.
    int current_order = 0;
    size_t temp = block->size;
    while (temp > 128) {  // smallest block is 128 bytes
        temp /= 2;
        current_order++;
    }

    // Compute the ideal block size for the new request.
    size_t needed = size + sizeof(MallocMetadata);
    int target_order = 0;
    size_t ideal_block_size = 128;
    while (ideal_block_size < needed && target_order < MAX_ORDER) {
        ideal_block_size *= 2;
        target_order++;
    }
    if (ideal_block_size < needed) {
        // Request is too large for buddy system.
        return nullptr;
    }

    // If the ideal block size is the same as the current block,
    // then we can keep the block in place.
    if (block->size == ideal_block_size) {
        return oldp;
    }

    // Otherwise, if the ideal block size is smaller than the current block’s size,
    void* newp = smalloc(size);
    if (!newp) {
        return nullptr;
    }

    size_t old_usable = block->size - sizeof(MallocMetadata);
    std::memmove(newp, oldp, std::min(size, old_usable));

    sfree(oldp);
    return newp;
}


size_t _num_free_blocks() { return num_free_blocks; }

size_t _num_free_bytes() { return num_free_bytes; }

size_t _num_allocated_blocks() { return num_allocated_blocks; }

size_t _num_allocated_bytes() { return num_allocated_bytes; }

size_t _num_meta_data_bytes() { return num_meta_data_bytes; }

size_t _size_meta_data() { return sizeof(MallocMetadata); }
