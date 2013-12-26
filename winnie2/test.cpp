// code by winnie_
#include <stdlib.h>

const int SIZE = 256;

void * first_free = NULL;

void new_block () {
    const int LARGE_BLOCK_SIZE = 1024 * 1000;
    char * block = (char *) malloc (LARGE_BLOCK_SIZE);
    const int N_BLOCKS = LARGE_BLOCK_SIZE / SIZE;
    for (int i = 0; i <N_BLOCKS; ++i) {
        * (void **) (block + i * SIZE) = ( i != N_BLOCKS - 1) ? (block + (i + 1) * SIZE): NULL;
    }
    first_free = block;
}

void * fixed_alloc () {
    if (! first_free)
        new_block ();

    void * result = first_free;
    first_free = * (void **) first_free;
    return result;
}

void fixed_free (void * ptr) {
    * (void **) ptr = first_free;
    first_free = ptr;
}

#include <stdio.h>

int main () {
    void * a1 = fixed_alloc ();
    void * a2 = fixed_alloc ();
    void * a3 = fixed_alloc ();

    fixed_free (a2);

    void * a4 = fixed_alloc ();
    void * a5 = fixed_alloc ();

    printf ("%p \n", a1);
    printf ("%p \n", a2);
    printf ("%p \n", a3);
    printf ("%p \n", a4);
    printf ("%p \n", a5);
}
