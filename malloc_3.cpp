#include <unistd.h>  
#include <cstddef>
#include <iostream>  
#include <cstring>


const long int MAX_SIZE=100000000;
const long int MAX_ORDER=10;

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
bool initialized = false;

void buddy_allocate(){

    int num=32, size=128*1024;

    void* ptr=sbrk(num*size);

    if (ptr == (void*) -1)
        return;

    MallocMetadata *tail = NULL, *head=NULL;
    for(int i=0;i<num;i++){

        MallocMetadata* current = (MallocMetadata*)ptr;

        current->size = size;
        current->is_free = false;
        current->next = NULL;
        current->prev = tail;
        if(tail)
            tail->next = current;
        tail=current;
        if(head==NULL)
            head=current;
        ptr = (char*)ptr + size;  // Move to the next block
        num_allocated_blocks++;
        num_allocated_bytes+=size;
        num_free_blocks++;
        num_free_bytes+=size;
        num_meta_data_bytes+=sizeof(MallocMetadata);

    }

    order_arr[MAX_ORDER]=head;
    return;

}

void* smalloc(size_t size){
    
    if(!initialized){
        buddy_allocate();
        initialized=true;
    }
    if(size==0||size>MAX_SIZE)
        return NULL;
    int order_size=64;
    for(int i=0; i<=MAX_ORDER; i++){
        order_size*=2;
        if(size>order_size){
            continue;
        }
        while(order_arr[i]==NULL){
            //TODO: implement challenge 1
            i++;//לא צריך להיות פה, רק לבינתיים
            continue;
        }

        MallocMetadata * current=order_arr[i];
        order_arr[i]=current->next;
        current->is_free = false; 
        num_free_blocks--;
        num_free_bytes -= current->size;
        return (void*)(current + 1); 

    }
    //TODO: implement challenge 3

    return NULL;
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
        //TODO: implement challenge 2
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
