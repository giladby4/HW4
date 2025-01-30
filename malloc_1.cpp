#include <unistd.h>  
#include <cstddef>
#include <iostream>  

const long int MAX_SIZE=100000000;

void* smalloc(size_t size){
    if(size==0||size>MAX_SIZE)
        return NULL;
    void * ptr=sbrk(size);
    if (ptr == (void*) -1)
        return NULL;
    return ptr;
}



int main(int argc, char* argv[]) {

    // בדיקה 1
    if (!smalloc(0)) {
        std::cout << "Test 1 passed\n";
    } else {
        std::cout << "Test 1 failed\n";
    }

    // בדיקה 2
    if (!smalloc(1000000000)) {
        std::cout << "Test 2 passed\n";
    } else {
        std::cout << "Test 2 failed\n";
    }

 // בדיקה 3
    if (smalloc(12)) {
        std::cout << "Test 3 passed\n";
    } else {
        std::cout << "Test 3 failed\n";
    }

// בדיקה 4  
    if (argc < 2) {
        return 0;
    }
    int overflag = std::atoi(argv[1]);  // המרת ארגומנט למספר
    if(overflag){
        int i=0;
        while(smalloc(100000000)){
            i++;
        }
        std::cout << "Test 4 passed after "<<i<<" iterations\n";
    }
//בדיקה 5
    if (argc < 3) {
        return 0;
    }

    int n = std::atoi(argv[2]);  // המרת ארגומנט למספר
    if (n <= 0) {
        std::cerr << "Invalid array size!\n";
        return 1;
    }
    int* arr = (int*)smalloc(n * sizeof(int));  
    for (int i = 0; i < n; i++) {
        arr[i] = i * 2;  // לדוגמה: מספרים זוגיים
    }
    std::cout << "Array allocated with " << n << " elements.\n";
    for (int i = 0; i < n; i++) {
        std::cout << arr[i] << " ";
    }
    std::cout << "\n";


    return 0;

}
