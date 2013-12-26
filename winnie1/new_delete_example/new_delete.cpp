
#include "winnie_alloc/winnie_alloc.h"

#include "new_delete.h"


#if defined(NDEBUG) && OP_NEW_DEBUG
#undef NDEBUG
#pragma message ("warning: NDEBUG and OP_NEW_DEBUG do not match. forcing #undef NDEBUG")
#endif
#include "assert.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
 
#include <new>
#include "string.h"


#if OP_NEW_USE_WINNIE_ALLOC

#define OP_NEW_DEBUG_BREAK __asm int 3
//#define OP_NEW_DEBUG_BREAK DebugBreak()

namespace
{

#if OP_NEW_DEBUG
  //begin of allocated block linked list
  //should be initialized at compile-time, before any new/delete operations.
  DebugHeader fake_header = { &fake_header, &fake_header }; 
#endif

#if OP_NEW_MULTITHREADING 


CRITICAL_SECTION OperatorNewCriticalSection;
bool OperatorNewCriticalSectionInitialized = false;


struct LockOpNew
{
  LockOpNew() 
  { 
    if (OperatorNewCriticalSectionInitialized) //allow one thread entering before initialization
      EnterCriticalSection(&OperatorNewCriticalSection); 
  }
  ~LockOpNew() 
  { 
    if (OperatorNewCriticalSectionInitialized) //allow one thread entering before initialization
      LeaveCriticalSection(&OperatorNewCriticalSection); 
  }
};

#endif //OP_NEW_MULTITHREADING



const unsigned guard_begin = 0x12345678;
const unsigned guard_end =   0x87654321;
const unsigned guard_after_delete = 0xDeadBeaf;

unsigned op_new_alloc_num = 0;

DWORD thread_id = 0; 

void *OpNewAlloc(size_t size)
{
  size_t real_size = size;

#if OP_NEW_DEBUG

#if (!OP_NEW_MULTITHREADING)
  //assert, that only one thread owns new/delete
  DWORD cur_thread_id = GetCurrentThreadId();
  if (thread_id)
  {
    if (thread_id != cur_thread_id)
      OP_NEW_ASSERT(
      !"Allocation from different threads detected in single thread mode."
      "You should enable OP_NEW_MULTITHREADING from new_detete_config.h");
  }
  else
  {
    thread_id = cur_thread_id;
  }
#endif //!OP_NEW_MULTITHREADING

  real_size+= sizeof(guard_begin) + sizeof(guard_end)  + sizeof(DebugHeader);
  unsigned block_num;
#endif  //OP_NEW_DEBUG

  void *p;
  {
#if OP_NEW_MULTITHREADING
    LockOpNew lock;

#if OP_NEW_DEBUG
  DWORD cur_thread_id = GetCurrentThreadId();
  if (thread_id)
  {
    if (thread_id != cur_thread_id && !OperatorNewCriticalSectionInitialized)
      OP_NEW_ASSERT(
      !"Allocation from different threads detected, but OperatorNewCriticalSectionInit was not called."
      "You should call OperatorNewCriticalSectionInit before any allocations from any second thread.");
  }
  else
  {
    thread_id = cur_thread_id;
  }
#endif //OP_NEW_DEBUG

#endif //OP_NEW_MULTITHREADING

  p= Winnie::Alloc(real_size);

#if OP_NEW_DEBUG
    block_num = ++op_new_alloc_num;
    {
      DebugHeader *ph = (DebugHeader *)p;

      ph->prev = &fake_header;
      ph->next = fake_header.next;

      fake_header.next->prev = ph;
      fake_header.next= ph;
    }
#endif
  } //end of multithread protection

#if OP_NEW_DEBUG
  if (block_num == op_new_break_alloc)
  {
    OP_NEW_DEBUG_BREAK;
  }  
  
  DebugHeader *ph = (DebugHeader *)p;
  ph->block_num = block_num;
  ph->size = size;
  unsigned *p_guard_begin = (unsigned *)(ph+1);
  *p_guard_begin = guard_begin;
  void *p_user_memory= (p_guard_begin + 1);
  unsigned *p_guard_end = (unsigned *)((char*)p_user_memory + size);
  *p_guard_end = guard_end;

  memset(p_user_memory, 0xCC, size);
  p = p_user_memory;
#endif

  return p;
}


void OpNewFree(void *p_user_memory)
{
  void *p = p_user_memory;
#if OP_NEW_DEBUG
  OpNewAssertIsValid(p_user_memory);

  DebugHeader *ph = OpNewGetHeader(p_user_memory);
  
  unsigned *p_guard_begin = (unsigned *)p_user_memory - 1;
  *p_guard_begin = guard_after_delete;

  p= ph;

#endif

  {
#if OP_NEW_MULTITHREADING
    LockOpNew lock;
#endif
#if OP_NEW_DEBUG
    ph->prev->next = ph->next;
    ph->next->prev = ph->prev;
    memset(ph, 0xDD, ph->size + sizeof(DebugHeader)+2*sizeof(unsigned));
#endif
    Winnie::Free(p);
  } //end of multithread protection
}


} //namespace

unsigned op_new_break_alloc = 0;

void OperatorNewCriticalSectionInit()
{
#if OP_NEW_MULTITHREADING
  if (!OperatorNewCriticalSectionInitialized)
  {
    InitializeCriticalSection(&OperatorNewCriticalSection);
    OperatorNewCriticalSectionInitialized = true;
  }
#endif
}

#if OP_NEW_DEBUG



void OpNewAssertIsValid(void *p_user_memory)
{
  OP_NEW_ASSERT(p_user_memory);
  unsigned *p_guard_begin = (unsigned *)p_user_memory - 1;
  DebugHeader *ph = (DebugHeader *)p_guard_begin-1;
  size_t size =ph->size;
  unsigned *p_guard_end = (unsigned *)((char *)p_user_memory + size);

  if (*p_guard_begin == guard_after_delete)
  {
    OP_NEW_ASSERT(!"possibly this block was already deleted");
  }


  if (*p_guard_begin != guard_begin)
  {
    if (*(p_guard_begin-1) == guard_begin) //MSVC place size of array in first 4 bytes
    {
      OP_NEW_ASSERT(
        !"likely, applying delete to memory, allocated by new TYPE[], where TYPE has non-trivial destructor  (use delete[])");
    }
    OP_NEW_ASSERT(!"deletion of invalid block or non-block, heap-buffer underrun, or other error");
  }

  if (*p_guard_end != guard_end)
  {
    OP_NEW_ASSERT(!"possibly heap-buffer overrun");
  }
}

void *OpNewBegin()
{
  return OpNewGetHeadersBlock(fake_header.next);
}

void *OpNewEnd()
{
  return OpNewGetHeadersBlock(&fake_header);
}

void *OpNewNextBlock(void *p_user_memory)
{
  return OpNewGetHeadersBlock(OpNewGetHeader(p_user_memory)->next);
}

void *OpNewGetBlockPointer(void *p)
{
  if (!p)
    return NULL;

  for (
    void 
      *pum = OpNewBegin(), 
      *p_end = OpNewEnd();
    pum != p_end;
    pum = OpNewNextBlock(pum))
  {
    DebugHeader *ph = OpNewGetHeader(pum);
    if ( p >= pum && p < ((char*)pum + ph->size))
      return pum;
  }

  return NULL;
}


void OpNewValidateHeap()
{
  for (
    void 
      *pum = OpNewBegin(), 
      *p_end = OpNewEnd();
    pum != p_end;
    pum = OpNewNextBlock(pum))
  {
    OpNewAssertIsValid(pum);
  }
}

void *OpNewGetHeadersBlock(DebugHeader *p_header)
{
  void *p_user_memory = (unsigned*)(p_header+1)+1;
  return p_user_memory;
}

DebugHeader *OpNewGetHeader(void *p_user_memory)
{
  OpNewAssertIsValid(p_user_memory);
  return (DebugHeader*)((unsigned*)p_user_memory-1)-1;
}

#endif //OP_NEW_DEBUG

void *operator new(size_t size)
{
  void *p = OpNewAlloc(size);
#if OP_NEW_DEBUG
  OpNewGetHeader(p)->is_array = false;
#endif
  return p;
}

void *operator new[](size_t size)
{
  void *p = OpNewAlloc(size);
#if OP_NEW_DEBUG
  OpNewGetHeader(p)->is_array = true;
#endif
  return p;
}

void operator delete(void *p) throw()
{
  if (p)
  {
#if OP_NEW_DEBUG
    int is_array= OpNewGetHeader(p)->is_array;
#endif
    OpNewFree(p);
#if OP_NEW_DEBUG
    OP_NEW_ASSERT(!is_array && "what allocated with new[], should be released by delete[], not delete");
#endif
  }
}

void operator delete[](void *p)throw()
{
  if (p)
  {
#if OP_NEW_DEBUG
    int is_array= OpNewGetHeader(p)->is_array;
#endif
    OpNewFree(p);
#if OP_NEW_DEBUG
    OP_NEW_ASSERT(is_array && "what allocated with new, should be released by delete, not delete[]");
#endif

  }
}

void *operator new(size_t size, const std::nothrow_t&) throw()
{
  try
  {
    void *p = OpNewAlloc(size);
#if OP_NEW_DEBUG
    OpNewGetHeader(p)->is_array = false;
#endif
    return p;
  }
  catch (...)
  {
    return NULL;
  }
}

void *operator new[](size_t size, const std::nothrow_t&) throw()
{
  try
  {
    void *p = OpNewAlloc(size);
#if OP_NEW_DEBUG
    OpNewGetHeader(p)->is_array = true;
#endif
    return p;
  }
  catch (...)
  {
    return NULL;
  }
}

#endif //OP_NEW_USE_WINNIE_ALLOC
