#include <stdlib.h>

#include "winnie.hpp"

struct fixed_allocator {

    enum { UPTO = 257 }; /*[0..256]*/
    void *_first_frees[UPTO];

    fixed_allocator() {
        for( int i = 0; i < UPTO; ++i )
            _first_frees[i] = 0;
    }

    void *new_block( const unsigned SIZE, const int LARGE_BLOCK_SIZE = 1024 * 1000 ) {
        char * block = (char *) malloc (LARGE_BLOCK_SIZE);
        const int N_BLOCKS = LARGE_BLOCK_SIZE / SIZE;
        for (int i = 0; i <N_BLOCKS; ++i) {
            * (void **) (block + i * SIZE) = ( i != N_BLOCKS - 1) ? (block + (i + 1) * SIZE) : 0;
        }
        return block;
    }

    void *fixed_malloc( int size ) {
        //assert( size < UPTO );

        void *&first_free = _first_frees[ size ];

        if( !first_free )
            first_free = new_block( size );

        void * result = first_free;
        first_free = * (void **) first_free;
        return result;
    }

    void fixed_free( void * ptr, int size ) {
        //assert( ptr );
        //assert( size < UPTO );

        void *&first_free = _first_frees[ size ];

        * (void **) ptr = first_free;
        first_free = ptr;
    }
} fa;

void *wmalloc( int granularity ) {
    return granularity > 256 ? malloc( granularity ) : fa.fixed_malloc( granularity );
}
void *wfree( void *ptr, int granularity ) {
    return granularity > 256 ? free( ptr ), 0 : fa.fixed_free( ptr, granularity ), 0;
}

