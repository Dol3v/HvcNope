
#define MY_ATTRIBUTE __attribute__((annotate("kernel")))

void do_math( int* x ) {
    *x += 5;
}

struct A {
    int* MY_ATTRIBUTE field;
    bool field2;
};

void* MY_ATTRIBUTE func() {
    return nullptr;
}

int main( void ) {
    int result = -1, val = 4;
    do_math( &val );

    A a = { nullptr, true };

    //int* __declspec(special) a = nullptr;
    void* MY_ATTRIBUTE b = nullptr;

    void* MY_ATTRIBUTE c = func();


    return result;
}