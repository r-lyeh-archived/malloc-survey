// http://stackoverflow.com/questions/1995871/how-to-write-a-thread-safe-and-efficient-lock-free-memory-allocator-in-c
// http://stackoverflow.com/users/3186/christopher
#include <windows.h>
#include <stdio.h>

#ifndef byte
#define byte unsigned char
#endif

typedef struct
{
    union{
        SLIST_ENTRY entry;
    void* list;
};
byte mem[];
} mem_block;

typedef struct
{
    SLIST_HEADER root;
} mem_block_list;

#define BUCKET_COUNT 4
#define BLOCKS_TO_ALLOCATE 16

namespace {

    mem_block_list Buckets[BUCKET_COUNT];

    bool init_buckets()
    {
        for( int i = 0; i < BUCKET_COUNT; ++i )
        {
            InitializeSListHead( &Buckets[i].root );
            for( int j = 0; j < BLOCKS_TO_ALLOCATE; ++j )
            {
                mem_block* p = (mem_block*) malloc( sizeof( mem_block ) + (0x1 << i) * 0x8 );
                //printf("creating block of %d bytes\n", (0x1 << i) * 0x8 );
                InterlockedPushEntrySList( &Buckets[i].root, &p->entry );
            }
        }
        return true;
    }

    const bool init = init_buckets();
}


void* balloc( size_t size ) {
    for( int i = 0; i < BUCKET_COUNT; ++i ) {
        if( size <= (0x1 << i) * 0x8 ) {
            mem_block* p = (mem_block*) InterlockedPopEntrySList( &Buckets[i].root );
            p->list = &Buckets[i];
            return ((byte *)p + sizeof( p->entry ));
        }
    }
    return 0;	// block too large
}

void  bfree( void* p ) {
    mem_block* block = (mem_block*) (((byte*)p) - sizeof( block->entry ));
    InterlockedPushEntrySList( &((mem_block_list*)block)->root, &block->entry );
}

// also,
// http://stackoverflow.com/questions/11749386/implement-own-memory-pool

typedef struct pool
{
  char * next;
  char * end;
} POOL;

POOL * pool_create( size_t size ) {
    POOL * p = (POOL*)malloc( size + sizeof(POOL) );
    p->next = (char*)&p[1];
    p->end = p->next + size;
    return p;
}

void pool_destroy( POOL *p ) {
    free(p);
}

size_t pool_available( POOL *p ) {
    return p->end - p->next;
}

void * pool_alloc( POOL *p, size_t size ) {
    if( pool_available(p) < size ) return NULL;
    void *mem = (void*)p->next;
    p->next += size;
    return mem;
}

