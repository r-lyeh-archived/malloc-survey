
//system_alloc.cpp
//OS-specific memory allocation functions
//this is part of winnie_alloc library
//you may rewrite this functions if you like

#include "system_alloc.h"

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>


void *SystemAlloc(size_t size)
{
  //return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  return malloc(size);
}

void SystemFree(void *p)
{
  //VirtualFree(p, 0, MEM_RELEASE);
  free(p);
}

