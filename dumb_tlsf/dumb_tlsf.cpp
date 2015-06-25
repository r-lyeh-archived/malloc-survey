


#ifndef DUMB_TLSF_ALLOCATOR
#include <stdlib.h>
#define DUMB_TLSF_ALLOCATOR malloc
#endif
#ifndef DUMB_TLSF_DEALLOCATOR
#include <stdlib.h>
static inline void DUMB_TLSF_DEALLOCATOR (void* p, size_t size) { free(p); }
#endif

#ifdef _WIN32
typedef size_t uint;
#else
#include <unistd.h>
#endif

namespace dumb_tlsf_private {

#ifdef _MSC_VER
#define constexpr const
#endif

static size_t alloc_size = 1<<20;

static constexpr uint segregate1 (uint esize, uint l1) {
    return (l1 << 2 | (esize >> (l1 - 2) & 3)) - 8;
}
static constexpr uint segregate (uint esize) {
#ifdef _MSC_VER
    unsigned long r;
    BitScanReverse((unsigned long*)&r, esize);
    return esize == 0 ? 0 : segregate1(esize, r);
#else
    return esize == 0 ? 0 : segregate1(esize, 31 - __builtin_clz(esize));
#endif
}
static constexpr uint esizeof (uint size) {
    return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
}

 // It'll probably be better to have fewer columns in this table...
static void* table [] = {  // Some of these near the beginning will be unused.
    NULL, NULL, NULL, NULL,  // 4
    NULL, NULL, NULL, NULL,  // 8
    NULL, NULL, NULL, NULL,  // 16
    NULL, NULL, NULL, NULL,  // 32
    NULL, NULL, NULL, NULL,  // 64
    NULL, NULL, NULL, NULL,  // 128
    NULL, NULL, NULL, NULL,  // 256
    NULL, NULL, NULL, NULL,  // 512
    NULL, NULL, NULL, NULL,  // 1024
    NULL, NULL, NULL, NULL,  // 2048
    NULL, NULL, NULL, NULL   // 4096
};
static const size_t max_size = 4096 + 3*1024;

struct Header {
    Header* prev;
    size_t size;
    char data [0];
};

static Header* res = NULL;
static char* limit = NULL;
static char* end = NULL;

static void* alloc_e (size_t esize, uint index) {
    void*& fl = table[index];
    if (fl) {
        void* r = fl;
        fl = *(void**)fl;
        return r;
    }
    else {
#ifndef DUMB_TLSF_PROMISE_NO_OVERFLOW
        if (limit + esize > end) {
            Header* new_res = (Header*)DUMB_TLSF_ALLOCATOR(alloc_size);
            new_res->prev = res;
            new_res->size = alloc_size;
            limit = new_res->data;
            res = new_res;
            end = (char*)res + alloc_size;
        }
#endif
        void* r = limit;
        limit += esize;
        return r;
    }
}

static void free_e (void* p, size_t esize, uint index) {
    *(void**)p = table[index];
    table[index] = p;
}

}

namespace dumb_tlsf {

static inline void* dalloc (size_t size) {
    using namespace dumb_tlsf_private;
    return size > max_size ? DUMB_TLSF_ALLOCATOR(size) : alloc_e(esizeof(size), segregate(esizeof(size)));
}

static inline void dfree (void* p, size_t size) {
    using namespace dumb_tlsf_private;
    return size > max_size ? DUMB_TLSF_DEALLOCATOR(p, size) : free_e(p, esizeof(size), segregate(esizeof(size)));
}

static void dreserve (size_t size) {
    using namespace dumb_tlsf_private;
    if (!limit) {
        res = (Header*)DUMB_TLSF_ALLOCATOR(size);
        res->prev = NULL;
        res->size = size;
        limit = res->data;
        end = (char*)res + size;
    }
}

static void dset_alloc_size (size_t size) {
    dumb_tlsf_private::alloc_size = size;
}

static size_t dreserved_memory () {
    using namespace dumb_tlsf_private;
    size_t total = 0;
    for (Header* blk = res; blk; blk = blk->prev)
        total += blk->size;
    return total;
}
static size_t dused_memory () {
    using namespace dumb_tlsf_private;
    return dreserved_memory() - (end - limit);
}

static void dapocalypse () {
    using namespace dumb_tlsf_private;
    Header* pres;
    for (; res; res = pres) {
        pres = res->prev;
        DUMB_TLSF_DEALLOCATOR(res, res->size);
    }
    limit = NULL;
    end = NULL;
    for (uint i = 0; i < sizeof(table)/sizeof(void*); i++) {
        table[i] = NULL;
    }
}

template <class T>
static inline T* dalloc () {
    return (T*)dalloc(sizeof(T));
}
template <class T, class... Args>
static inline T* dnew (Args... args) {
    T* r = dalloc<T>();
    r->T(args...);
    return r;
}
template <class T>
static inline void dfree (T* p) {
    dfree(p, sizeof(T));
}
template <class T>
static inline void ddelete (T* p) {
    p->~T();
    dfree(p);
}

static inline void* dsalloc (size_t size) {
    size_t* p = (size_t*)dalloc(size + sizeof(size_t));
    *p = size;
    return p + 1;
}

static inline void dsfree (void* p) {
    size_t* sp = (size_t*)p - 1;
    dfree(sp, *sp);
}

template <class T>
static inline T* dsalloc () {
    return (T*)dsalloc(sizeof(T*));
}

template <class T, class... Args>
static inline T* dsnew (Args... args) {
    T* r = dsalloc<T>();
    r->T(args...);
    return r;
}

template <class T>
static inline void dsdelete (T* p) {
    p->~T();
    dsfree(p);
}

struct _DS {
    void* operator new (size_t size) { return dumb_tlsf::dsalloc(size); }
    void* operator new[] (size_t size) { return dumb_tlsf::dsalloc(size); }
    void operator delete (void* p) { return dumb_tlsf::dsfree(p); }
    void operator delete[] (void* p) { return dumb_tlsf::dsfree(p); }
} DS;

}

void* operator new (size_t size, dumb_tlsf::_DS ds) { return dumb_tlsf::dsalloc(size); }
void operator delete (void* p, dumb_tlsf::_DS ds) { return dumb_tlsf::dsfree(p); }
