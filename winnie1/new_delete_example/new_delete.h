
#ifndef NEW_DELETE_HEADER_
#define NEW_DELETE_HEADER_

#ifdef _MSC_VER
#  pragma once
#endif

#include "new_delete_config.h"

#if OP_NEW_USE_WINNIE_ALLOC

/** Should be called before any concurrent allocations in other threads 
@see OP_NEW_MULTITHREADING
*/
void OperatorNewCriticalSectionInit(); 


//debug stuff
#if OP_NEW_DEBUG

/** this structure added to each block, if defined OP_NEW_DEBUG. 
You can retrieve it by OpNewGetHeader function. Also you can add your own members.
@see OpNewGetHeader*/
struct DebugHeader
{
  /** pointer to headers of next and previos blocks. */ 
  DebugHeader *prev, *next;
  /** size of requested block size. Do NOT include size of guards and header */
  size_t size;
  /** number of allocation */
  unsigned block_num;
  /** was allocated by new[] or new? */
  int is_array;
//---------------------------------
//you can add your own fields here.
};

/**returns pointer( "iterator" ) to the first block */
void *OpNewBegin();
/**returns pointer ( "iterator" ) to fake last block, which followed (see OpNewNextBlock)after real last block. */
void *OpNewEnd();
/**returns next block*/
void *OpNewNextBlock(void *p_user_memory);

/** converts ANY valid pointer to memory, allocated whith new/delete to 
beggining of the block. 
WARNING: GetBlockPointer can take much of time, because it searchs for 
block in entire list of allocated blocks. If p is NULL, returns NULL. If there is no 
such block, returns NULL. */
void *OpNewGetBlockPointer(void *p);

/** apply OpNewAssertIsValid to each block in list of blocks */
void OpNewValidateHeap();

/**Gets header of block */
DebugHeader *OpNewGetHeader(void *p_user_memory);

/**Gets block of header*/
void *OpNewGetHeadersBlock(DebugHeader *p_header);

/** asserts that p_user_memory is valid allocated block with uncorrupted guards. */
void OpNewAssertIsValid(void *p_user_memory);

/** Break in to debuger, if allocation block has number OpNewBreakAlloc. 
You can change this variable from watch window, for example. */
extern unsigned op_new_break_alloc; 

#  endif //OP_NEW_DEBUG

#endif //OP_NEW_USE_WINNIE_ALLOC

#endif //NEW_DELETE_HEADER_
