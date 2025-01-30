#include "malloc_3.cpp"

#include <iostream>
#include <cstdlib>  // For atoi
#include <cstring>  // For memmove

void print_stats() {
    std::cout << "Free Blocks: " << _num_free_blocks() << "\n";
    std::cout << "Free Bytes: " << _num_free_bytes() << "\n";
    std::cout << "Allocated Blocks: " << _num_allocated_blocks() << "\n";
    std::cout << "Allocated Bytes: " << _num_allocated_bytes() << "\n";
    std::cout << "Metadata Bytes: " << _num_meta_data_bytes() << "\n";
    std::cout << "Metadata Size: " << _size_meta_data() << "\n";
}

int main() {

    std::cout << "==== Testing smalloc ====\n";
    void* p1 = smalloc(16);
    std::cout << (p1 ? "smalloc(16) succeeded\n" : "smalloc(16) failed\n");

    void* p2 = smalloc(MAX_SIZE + 1);
    std::cout << (p2 ? "smalloc(MAX_SIZE+1) failed!\n" : "smalloc(MAX_SIZE+1) correctly returned NULL\n");

    void* p3 = smalloc(0);
    std::cout << (p3 ? "smalloc(0) failed!\n" : "smalloc(0) correctly returned NULL\n");

    print_stats();

    std::cout << "\n==== Testing scalloc ====\n";
    int* arr1 = (int*)scalloc(10, sizeof(int));
    if (arr1) {
        std::cout << "scalloc(10, int) succeeded\n";
        bool zero_initialized = true;
        for (int i = 0; i < 10; i++) {
            if (arr1[i] != 0) {
                zero_initialized = false;
                break;
            }
        }
        std::cout << (zero_initialized ? "Memory initialized to 0\n" : "Memory NOT initialized to 0!\n");
    }

    int* arr2 = (int*)scalloc(5, sizeof(int));
    std::cout << (arr2 ? "scalloc(5, int) succeeded\n" : "scalloc(5, int) failed\n");

    print_stats();

    std::cout << "\n==== Testing sfree ====\n";
    sfree(p1);
    std::cout << "Freed p1\n";
    print_stats();

    sfree(p1);  // Double free
    std::cout << "Tried to free p1 again (should do nothing)\n";
    print_stats();

    std::cout << "\n==== Testing srealloc ====\n";
    int* arr3 = (int*)smalloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) arr3[i] = i * 10;

    arr3 = (int*)srealloc(arr3, 10 * sizeof(int));
    std::cout << (arr3 ? "srealloc expanded array\n" : "srealloc failed\n");

    bool data_ok = true;
    for (int i = 0; i < 5; i++) {
        if (arr3[i] != i * 10) {
            data_ok = false;
            break;
        }
    }
    std::cout << (data_ok ? "Data preserved after expansion\n" : "Data lost!\n");
    print_stats();

    arr3 = (int*)srealloc(arr3, 2 * sizeof(int));
    std::cout << (arr3 ? "srealloc shrunk array\n" : "srealloc failed\n");
    print_stats();

    arr3 = (int*)srealloc(NULL, 5 * sizeof(int));
    std::cout << (arr3 ? "srealloc(NULL) behaves like smalloc\n" : "srealloc(NULL) failed\n");

    print_stats();

    return 0;
}
