// r-lyeh
// based on code by winnie_

#pragma once

#ifdef max
#undef max
#endif

void *wmalloc( int granularity );
void *wfree( void *ptr, int granularity );

namespace {

template<typename T>
T *wmake() {
    return (T *)wmalloc( sizeof(T) );
}

#define tp template
#define ty typename
tp<ty T>                             T* wnew()                                                         { return new (wmake<T>()) T(); }
tp<ty T, ty A1>                      T* wnew( const A1 &a1 )                                           { return new (wmake<T>()) T( a1 ); }
tp<ty T, ty A1, ty A2>               T* wnew( const A1 &a1, const A2 &a2 )                             { return new (wmake<T>()) T( a1, a2 ); }
tp<ty T, ty A1, ty A2, ty A3>        T* wnew( const A1 &a1, const A2 &a2, const A3 &a3 )               { return new (wmake<T>()) T( a1, a2, a3 ); }
tp<ty T, ty A1, ty A2, ty A3, ty A4> T* wnew( const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4 ) { return new (wmake<T>()) T( a1, a2, a3, a4 ); }
#undef ty
#undef tp

template<typename T>
void wdelete( T*& t ) {
    if( t ) {
        t->~T();
        wfree( t, sizeof(T) );
        t = 0;
    }
}

}

#include <memory>

namespace winnie2 {
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
        pointer temp = (pointer)wmalloc(s * sizeof(T));
        if (temp == NULL)
            throw std::bad_alloc();
        return temp;
    }

    void deallocate(pointer p, size_type sz) {
        wfree(p, sz);
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

