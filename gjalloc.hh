#ifndef BLOCK_ALLOCATOR_HH
#include <new>
#include <vector>
#include <cassert>
#include <iostream> // DEBUG
#include "gjalloc.h"
#define BLOCK_ALLOCATOR_HH


template <size_t BA_N, size_t BLOCKS> class GJAlloc_Singleton
{
    struct MV {
	block_allocator allocator;
	long unsigned int cnt;

	MV() {
	    cnt = 0;
	}
    };
    static MV mv;

public:
    static void register_alloc() {
	if (unlikely(!mv.cnt++))
	    ba_init(&mv.allocator, BA_N, BLOCKS);
    }

    static void unregister_alloc() {
	if (unlikely(!--mv.cnt))
	    ba_destroy(&mv.allocator);
    }

    static void *allocate() {
	return ba_alloc(&mv.allocator);
    }

    static void deallocate(void *p) {
	ba_free(&mv.allocator, p);
    }

    template <typename T> static void construct(void *p, const T &val) {
	std::cerr << "WARNING: static construct has been called." << std::endl;
	new ((void*)p) T(val);
    }

    template <typename T> static void destroy(void *p) {
	((T*)p)->~T();
    }


    static uint32_t num_pages() {
	return mv.allocator.num_pages;
    }

    static size_t count_blocks() {
	size_t num, size;
	ba_count_all(&(mv.allocator), &num, &size);
	return num;
    }

    static size_t count_bytes() {
	size_t num, size;
	ba_count_all(&(mv.allocator), &num, &size);
	return size;
    }
};

template <size_t BA_N, size_t BLOCKS> typename GJAlloc_Singleton<BA_N,BLOCKS>::MV GJAlloc_Singleton<BA_N,BLOCKS>::mv;

template <typename T, size_t BLOCKS=0, typename ptr_type=T*> class GJAlloc {
public:
    typedef ptr_type pointer;
    typedef const ptr_type const_pointer;
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    pointer allocate(size_type n, const void* = 0) {
	assert(n == 1);
	//return static_cast<pointer>(GJAlloc_Singleton<sizeof(T)>::singleton.allocate());
	//return static_cast<pointer>(singleton.allocate());
	pointer ptr(static_cast<value_type*>(GJAlloc_Singleton<sizeof(T),BLOCKS>::allocate()));
	return ptr;
    }

    void deallocate(pointer p, size_type n) {
	assert(n == 1);

	GJAlloc_Singleton<sizeof(T),BLOCKS>::deallocate(static_cast<void*>(p));
    }

    void construct(pointer p, const T &val) {
	new ((void*)p) T(val);
    }

    void destroy(pointer p) {
	((T*)p)->~T();
    }

    template <typename U> struct rebind {
	typedef GJAlloc<U> other;
    };

    GJAlloc() throw() {
	GJAlloc_Singleton<sizeof(T),BLOCKS>::register_alloc();
    }
    GJAlloc(const GJAlloc &a) throw() {
	GJAlloc_Singleton<sizeof(T),BLOCKS>::register_alloc();
    }
    template <typename U> GJAlloc(const GJAlloc<U> &a) throw() {
	GJAlloc_Singleton<sizeof(T),BLOCKS>::register_alloc();
    }
    ~GJAlloc() throw() {
	GJAlloc_Singleton<sizeof(T),BLOCKS>::unregister_alloc();
    }

    size_type max_size() {
	return 1;
    }

    uint32_t num_pages() {
	return GJAlloc_Singleton<sizeof(T),BLOCKS>::num_pages();
    }

    size_t count_blocks() {
	return GJAlloc_Singleton<sizeof(T),BLOCKS>::count_blocks();
    }

    size_t count_bytes() {
	return GJAlloc_Singleton<sizeof(T),BLOCKS>::count_bytes();
    }
};

template <class T1, class T2>
bool operator==(const GJAlloc<T1>&, const GJAlloc<T2>&) throw() {
    return sizeof(T1)==sizeof(T2);
}

template <class T1, class T2>
bool operator!=(const GJAlloc<T1>&, const GJAlloc<T2>&) throw() {
    return sizeof(T1)!=sizeof(T2);
}

#endif
