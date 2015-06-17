// based on code by Jerry Coffin (most likely Public Domain)
// - rlyeh

#pragma once

#include <stdlib.h>
#include <new>
#include <limits>
#include <memory>

#include "tlsf.h"

namespace tlsf {

struct pool {
    tlsf_t tlsf;

    pool() {
        enum { POOL_SIZE = 256 * 1024 * 1024 };
        tlsf = tlsf_create_with_pool( calloc(1,POOL_SIZE), POOL_SIZE );
    }

    static tlsf_t &get() {
        static pool _;
        return _.tlsf;
    }
};

template <class T>
struct allocator {
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    template <class U> struct rebind { typedef allocator<U> other; };
    allocator() throw() { 
    }
    allocator( const allocator&other ) throw() { 
    }
    template <class U> allocator( const allocator<U>&other ) throw(){ 
    }
    ~allocator() throw() {
    }

    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    pointer allocate(size_type s, void const * = 0) {
        if (0 == s)
            return NULL;
        pointer temp = (pointer)tlsf_malloc(pool::get(), s * sizeof(T));
        if (temp == NULL)
            throw std::bad_alloc();
        return temp;
    }

    void deallocate(pointer p, size_type) {
        tlsf_free(pool::get(), p);
    }

    size_type max_size() const throw() {
        return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    void construct(pointer p, const T& val) {
        new((void *)p) T(val);
    }

    void destroy(pointer p) {
        p->~T();
    }
};
}
