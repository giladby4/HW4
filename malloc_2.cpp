#include <unistd.h>  
#include <cstddef>
#include <iostream>  
#include <cstring>


const long int MAX_SIZE=100000000;
struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* head = nullptr;
size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;
size_t num_meta_data_bytes = 0;



void* smalloc(size_t size){
    if(size==0||size>MAX_SIZE)
        return NULL;
    MallocMetadata* current = head;
    MallocMetadata* last_one = NULL;

    while (current) {
        last_one=current;
        if (current->is_free && current->size >= size) {
            current->is_free = false; 
            num_free_blocks--;
            num_free_bytes -= current->size;
            return (void*)(current + 1); 
        }
        last_one=current;
        current = current->next; //if isnt free
    }
    //if we got here, didnt found free block

    void * ptr=sbrk(sizeof(MallocMetadata) + size);
    if (ptr == (void*) -1)
        return NULL;
    //fill in the new meta data
    MallocMetadata* metadata = (MallocMetadata*)ptr;
    metadata->size = size;
    metadata->is_free = false;
    metadata->next = NULL;
    //add it to the end of the list
    metadata->prev = last_one;
    if (!head) {
        head = metadata;
    }
    else{
        last_one->next = metadata;
    }
    num_allocated_blocks++;
    num_allocated_bytes+=size;
    num_meta_data_bytes+=sizeof(MallocMetadata);
    return (void *)(metadata+1);
}

void* scalloc(size_t num, size_t size){
    void* ptr= smalloc(num*size);
    if(!ptr)
        return NULL;
    for (size_t i = 0; i < num * size; ++i) {
        ((unsigned char*)ptr)[i] = 0;
    }
    return ptr;
}

void sfree(void* p){
    if(!p){
        return;
    }
    MallocMetadata* metadata = (MallocMetadata*)p - 1;
    if(metadata&&!(metadata->is_free)){
        metadata->is_free = true;
        num_free_blocks++;
        num_free_bytes+=metadata->size;
    }
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
