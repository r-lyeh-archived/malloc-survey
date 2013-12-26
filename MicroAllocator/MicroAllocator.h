#ifndef MICRO_ALLOCATOR_H
#define MICRO_ALLOCATOR_H

/*!
**
** Copyright (c) 2009 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** If you wish to contact me you can use the following methods:
**
** Skype ID: jratcliff63367
** email: jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/


// This code snippet provides a high speed micro-allocator.
//
// The concept is that you reserve an initial bank of memory for small allocations.  Ideally a megabyte or two.
// The amount of memory reserved is equal to chunkSize*6
//
// All micro-allocations are split into 6 seperate pools.
//    They are: 0-8 bytes
//              9-32 bytes
//              33-64 bytes
//              65-128 bytes
//              129-256 bytes
//
// On creation of the micro-allocation system you preserve a certiain amount of memory for each of these banks.
//
// The user provides a heap interface to callback for additional memory as needed.
//
// In most cases allocations are order-N and frees are order-N as well.
//
// The larger a buffer you provide, the closer to 'order-N' the allocator behaves.
//
// This kind of a micro-allocator is ideal for use with STL as it does many tiny allocations.
// All allocations are 16 byte aligned (with the exception of the 8 byte allocations, which are 8 byte aligned every other one).
//

namespace MICRO_ALLOCATOR
{

class HeapManager;

// creates a heap manager that uses micro-allocations for all allocations < 256 bytes and standard malloc/free for anything larger.
HeapManager * createHeapManager(size_t defaultChunkSize=32768);
void          releaseHeapManager(HeapManager *heap);

// about 10% faster than using the virtual interface, inlines the functions as much as possible.
void * heap_malloc(HeapManager *hm,size_t size);
void   heap_free(HeapManager *hm,void *p);
void * heap_realloc(HeapManager *hm,void *oldMem,size_t newSize);

}; // end of namespace

using namespace MICRO_ALLOCATOR;

// Jerry Coffin
//#pragma once

#include <stdlib.h>
#include <new>
#include <limits>

namespace micro {
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
    allocator() throw() {}
    allocator(const allocator&) throw() {}

    template <class U> allocator(const allocator<U>&) throw(){}

    ~allocator() throw() {}

    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    pointer allocate(size_type s, void const * = 0) {
        if (0 == s)
            return NULL;
        pointer temp = (pointer)heap_malloc( get(), (s * sizeof(T)) );
        if (temp == NULL)
            throw std::bad_alloc();
        return temp;
    }

    void deallocate(pointer p, size_type) {
        heap_free( get(), p );
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

    static HeapManager *get() {
        static HeapManager *hmg = createHeapManager(5*1024*1024); // 4mb max for each heap when microallocating (<=1024 bytes)
        return hmg;
    }
};

}




#endif
