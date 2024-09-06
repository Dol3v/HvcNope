
#define MY_ATTRIBUTE __attribute__((annotate("kernel")))

void do_math( int* x ) {
    *x += 5;
}

int main( void ) {
    int result = -1, val = 4;
    do_math( &val );

    //int* __declspec(special) a = nullptr;
    void* MY_ATTRIBUTE b = nullptr;

    return result;
}