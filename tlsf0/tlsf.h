/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.4.6
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2008, 2007, 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 */

#ifndef _TLSF_H_
#define _TLSF_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t init_memory_pool(size_t, void *);
size_t get_used_size(void *);
size_t get_max_size(void *);
void destroy_memory_pool(void *);
size_t add_new_area(void *, size_t, void *);
void *malloc_ex(size_t, void *);
void free_ex(void *, void *);
void *realloc_ex(void *, size_t, void *);
void *calloc_ex(size_t, size_t, void *);

void *tlsf0_malloc(size_t size);
void tlsf0_free(void *ptr);
void *tlsf0_realloc(void *ptr, size_t size);
void *tlsf0_calloc(size_t nelem, size_t elem_size);

#ifdef __cplusplus
}
#endif

#endif
