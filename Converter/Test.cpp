
//#include "a/hello.h"

//#define _AMD64_
//#include "ntddk.h"

#include <intrin.h> // Include this header for intrinsics
#include "a/hello.h"

#define KERNEL      __attribute__((annotate("kernel")))
#define KERNEL_GS   __attribute__((annotate("kernel_gs")))

void do_math( int* x ) {
    *x += 5 * __readgsbyte(3);
}


void* KERNEL func() {
    return nullptr;
}

unsigned char KERNEL_GS func3() {
    return __readgsbyte( 0 );
}

int main( void ) {
    int result = -1, val = 4;
    do_math( &val );


    //int* __declspec(special) a = nullptr;
    //void* MY_ATTRIBUTE b = nullptr;

    //void* MY_ATTRIBUTE c = func();

    unsigned long long val2 = __readmsr( 0x600 );

    int d = func2();

    return result;
}