
#include <stdio.h>
#ifdef _WIN32
typedef size_t uint;
#else
#include <unistd.h>
#endif
//#define DUMB_TLSF_PROMISE_NO_OVERFLOW
#include "dumb_tlsf.cpp"

using namespace dumb_tlsf;

struct big : _DS {
    char x [4096];
};

big* bigs [100000];


int main () {
    dreserve(100001 * (sizeof(big) + sizeof(size_t)));
    uint i;
    for (i = 0; i < 100000; i++) {
        bigs[i] = new big;
        bigs[i]->x[0] = 1;
    }
    printf("%u %ld %ld\n", i, dused_memory(), dreserved_memory());
    for (i = 0; i < 100000; i++) {
        delete bigs[i];
    }
    printf("%u %ld %ld\n", i, dused_memory(), dreserved_memory());
    for (i = 0; i < 100000; i++) {
        bigs[i] = new big;
        bigs[i]->x[0] = 1;
    }
    printf("%u %ld %ld\n", i, dused_memory(), dreserved_memory());
    dapocalypse();
    printf("%u %ld %ld\n", i, dused_memory(), dreserved_memory());
    //sleep(100000);
    return 0;
}


