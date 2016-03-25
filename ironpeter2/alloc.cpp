#include <windows.h>
 
typedef unsigned char uint8;
typedef unsigned short uint16;
 
#define PLATFORM_SIZE 65536
#define GRAN_ALLOC 4
#define PLATFORM_BINS 32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t allocs = 0;
size_t bremap[PLATFORM_SIZE / GRAN_ALLOC / 2];
size_t bsizes[PLATFORM_SIZE / GRAN_ALLOC / 2];
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static struct U
{
    U()
    {
        size_t sizes[256];
        size_t curr = GRAN_ALLOC;
 
        for( size_t i = 0; i < 256; ++i )
        {
            sizes[i] = curr;
            size_t ncurr = ( curr * 4  ) / 3;
            ncurr -= ( ncurr % GRAN_ALLOC );
            if( ncurr == curr )
            {
                ncurr += GRAN_ALLOC;
            }
            curr = ncurr;
        }
 
        size_t j = 0;
        for( size_t i = 0; i < PLATFORM_SIZE / GRAN_ALLOC / 2; ++i )
        {
            while( sizes[j] < i * GRAN_ALLOC + GRAN_ALLOC )
            {
                ++j;
            }
 
            bremap[i] = j;
            bsizes[i] = sizes[j];
            assert( j < PLATFORM_BINS && "bad bins" );
        }
    }
} unnamed;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *platform_alloc( size_t count = 1 )
{
    ++allocs;
    void *data = VirtualAlloc( 0, PLATFORM_SIZE * count, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    assert( ( (size_t)data % PLATFORM_SIZE ) == 0 );
    return data;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void platform_free( void *data )
{
    if( data == 0 )
    {
        return;
    }
    --allocs;
    assert( ( (size_t)data % PLATFORM_SIZE ) == 0 );
    VirtualFree( data, 0, MEM_RELEASE );
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct _mspace
{
    struct node
    {
        struct node_header
        {
            node   *next;
            size_t  usedObjs;
            size_t  index;
            void   *firstObject;
        }; 
 
        node_header header;
        void        *temp;
   
        node( size_t granularity, size_t index )
        {
            header.index = index;
            size_t limit = ( PLATFORM_SIZE - sizeof( node_header ) ) / ( granularity );
            header.firstObject = (void *)( &temp );
 
            void *tail = header.firstObject;
 
            size_t i;
 
            for( i = 0; i + 1 < limit; ++i )
            {
                *(void **)tail = (uint8 *)header.firstObject + ( i + 1 ) * granularity;
                tail = *(void **)tail;
            }
 
            tail = (uint8 *)header.firstObject + i * granularity;
            *(void **)tail = 0;
 
            header.usedObjs = 0;
        }
 
        void *alloc()
        {
            ++header.usedObjs;
            void *res = header.firstObject;
            header.firstObject = *(void **)header.firstObject;
            return  res;
        }
 
        void free( void *data )
        {
            --header.usedObjs;
            *(void **)data = header.firstObject;
            header.firstObject = data;
 
        }
    };
 
    node *nodes[PLATFORM_BINS];
 
 
    void clean_up()
    {
        for( size_t i = 0; i < PLATFORM_BINS; ++i )
        {
            node *next = nodes[i];
 
            nodes[i] = 0;
 
            while( next )
            {
                node *nnext = next->header.next;
                if( next->header.usedObjs == 0 )
                {
                    platform_free( next );
                }
                else
                {
                    next->header.next = nodes[i];
                    nodes[i] = next;
                }   
                next = nnext;
            }
        }
    }
 
    void free( void *p )
    {
        node *dest = (node *)( void * ) ( size_t( p ) & ~65535 );
 
        if( dest == p )
        {
            platform_free( p );
            return;
        }
       
        if( dest->header.firstObject == 0 )
        {
            dest->header.next = nodes[dest->header.index];
            nodes[dest->header.index] = dest;
        }
       
        dest->free( p );
 
        if( dest->header.usedObjs == 0 )
        {
            clean_up();
        }
    }
 
    void *malloc( size_t size )
    {
        if( size > PLATFORM_SIZE / 2 )
        {
            return platform_alloc( ( size - 1 ) / PLATFORM_SIZE + 1 );
        }
       
        size_t msb, mss;
 
        size = size == 0 ? 0 : ( size - 1 ) / GRAN_ALLOC;
 
        msb = bremap[size];
        mss = bsizes[size];
   
        node *dest = nodes[msb];
 
        if( dest == 0 )
        {
            node *next =  new( platform_alloc() ) node( mss, msb );
            next->header.next = nodes[msb];
            nodes[msb] = next;
            dest = next;
        }
 
        void *result = dest->alloc();
       
        if( dest->header.firstObject == 0 )
        {
            nodes[msb] = dest->header.next;
        }
 
        return result;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 _mspace ms;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void * __cdecl operator new( size_t n )
{
    return ms.malloc( n );
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void __cdecl operator delete( void *p )
{
    return ms.free( p );
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void * __cdecl operator new[]( size_t n )
{
    return ms.malloc( n );
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void __cdecl operator delete[]( void * p )
{
    return ms.free( p );
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 