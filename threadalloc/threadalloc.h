#ifndef __THREAD_ALLOC_H__
#define __THREAD_ALLOC_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* thread_alloc(size_t size);
void* thread_realloc(void* addr, size_t size);
void  thread_free(void* addr);

#ifdef __cplusplus
}

#include <new>

#ifdef REDEFINE_DEFAULT_NEW_OPERATOR

inline void* operator new(size_t size) throw(std::bad_alloc) { return thread_alloc(size); }
inline void operator delete(void* addr) throw() { thread_free(addr); }

inline void* operator new[](size_t size) throw(std::bad_alloc) { return thread_alloc(size); }
inline void operator delete[](void* addr) throw() { thread_free(addr); }

#endif

#endif

#endif

