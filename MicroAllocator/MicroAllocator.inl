/*!
** Copyright (c) 2009 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
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

#include <new>
#include <assert.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#endif

#if defined(__APPLE__) || defined(LINUX)
#include <pthread.h>
#endif

#include "MicroAllocator.h"

#ifndef mc_malloc
#define mc_malloc  ::malloc
#endif

#ifndef mc_realloc
#define mc_realloc ::realloc
#endif

#ifndef mc_free
#define mc_free    ::free
#endif

#ifdef _WIN32
        typedef unsigned __int64        size64;
#else
        // linux, apple, ...
        typedef unsigned long long      size64;
#endif

#pragma warning(disable:4100)

#ifndef $debug
#if defined(NDEBUG) || defined(_NDEBUG)
#define $debug(...)
#else
#define $debug(...) __VA_ARGS__
#endif
#endif


namespace MICRO_ALLOCATOR
{
  enum settings {
    MAX_ALLOCATION_SIZE = 1024,
    MAX_ALLOCATORS = 17
  };


  // user provided heap allocator
  class MicroHeap
  {
  public:
    virtual void * micro_malloc(size_t size) = 0;
    virtual void   micro_free(void *p) = 0;
    virtual void * micro_realloc(void *oldMen,size_t newSize) = 0;
  };

  class MemoryChunk;

  class MicroAllocator
  {
  public:
    virtual void *          malloc(size_t size) = 0;
    virtual void            free(void *p,MemoryChunk *chunk) = 0; // free relative to previously located MemoryChunk
    virtual MemoryChunk *   isMicroAlloc(const void *p) = 0; // returns pointer to the chunk this memory belongs to, or null if not a micro-allocated block.
    virtual size_t          getChunkSize(MemoryChunk *chunk) = 0;
  };

  MicroAllocator *createMicroAllocator(MicroHeap *heap,size_t chunkSize=32768); // initial chunk size 32k per block.
  void            releaseMicroAllocator(MicroAllocator *m);

  class HeapManager
  {
  public:
    virtual void * heap_malloc(size_t size) = 0;
    virtual void   heap_free(void *p) = 0;
    virtual void * heap_realloc(void *oldMem,size_t newSize) = 0;
  };
}

namespace MICRO_ALLOCATOR
{

//==================================================================================
class MemMutex
{
    public:
        MemMutex(void){
#if defined(_WIN32) || defined(_XBOX)
    InitializeCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
    pthread_mutexattr_t mta;
    pthread_mutexattr_init(&mta);
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_Mutex, &mta);
    pthread_mutexattr_destroy(&mta);
#endif
}
        ~MemMutex(void){
#if defined(_WIN32) || defined(_XBOX)
    DeleteCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
    pthread_mutex_destroy(&m_Mutex);
#endif
}

    public:
        // Blocking Lock.
        void Lock(void){
#if defined(_WIN32) || defined(_XBOX)
    EnterCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
    pthread_mutex_lock(&m_Mutex);
#endif
}

        // Unlock.
        void Unlock(void){
#if defined(_WIN32) || defined(_XBOX)
    LeaveCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
    pthread_mutex_unlock(&m_Mutex);
#endif
}

private:
        #if defined(_WIN32) || defined(_XBOX)
        CRITICAL_SECTION m_Mutex;
        #elif defined(__APPLE__) || defined(LINUX)
        pthread_mutex_t  m_Mutex;
        #endif
};


struct ChunkHeader
{
  ChunkHeader   *mNextChunk;
};

// interface to add and remove new chunks to the master list.
class MicroChunkUpdate
{
public:
  virtual void addMicroChunk(unsigned char *memStart,unsigned char *memEnd,MemoryChunk *chunk) = 0;
  virtual void removeMicroChunk(MemoryChunk *chunk) = 0;
};

class MemoryHeader
{
public:
  MemoryHeader *mNext;
};

// a single fixed size chunk for micro-allocations.
class MemoryChunk
{
public:
  MemoryChunk(void)
  {
    mData       = 0;
    mDataEnd    = 0;
    mUsedCount  = 0;
    mFreeList   = 0;
    mMyHeap     = false;
    mChunkSize  = 0;
  }

  unsigned char *init(unsigned char *chunkBase,size_t chunkSize,size_t maxChunks)
  {
    mChunkSize  = chunkSize;
    mData     =  chunkBase;
    mDataEnd  = mData+(chunkSize*maxChunks);
    mFreeList = (MemoryHeader *) mData;
    MemoryHeader *scan = mFreeList;
    unsigned char *data = mData;
    data+=chunkSize;
    for (size_t i=0; i<(maxChunks-1); i++)
    {
        MemoryHeader *next = (MemoryHeader *)data;
        scan->mNext = next;
        data+=chunkSize;
        scan = next;
    }
    scan->mNext = 0;
    return mDataEnd;
  }

  inline void * allocate(MicroHeap *heap,size_t chunkSize,size_t maxChunks,MicroChunkUpdate *update)
  {
    void *ret = 0;

    if ( mData == 0 )
    {
        mMyHeap = true;
        mData = (unsigned char *)heap->micro_malloc( chunkSize * maxChunks );
        init(mData,chunkSize,maxChunks);
        update->addMicroChunk(mData,mDataEnd,this);
    }
    if ( mFreeList )
    {
        mUsedCount++;
        ret = mFreeList;
        mFreeList = mFreeList->mNext;
    }
    return ret;
  }

  inline void deallocate(void *p,MicroHeap *heap,MicroChunkUpdate *update)
  {
$debug(
    assert(mUsedCount);
    unsigned char *s = (unsigned char *)p;
    assert( s >= mData && s < mDataEnd );
)
    MemoryHeader *mh = mFreeList;
    mFreeList = (MemoryHeader *)p;
    mFreeList->mNext = mh;
    mUsedCount--;
    if ( mUsedCount == 0 && mMyHeap  ) // free the heap back to the application if we are done with this.
    {
        update->removeMicroChunk(this);
        heap->micro_free(mData);
        mMyHeap = false;
        mData   = 0;
        mDataEnd = 0;
        mFreeList = 0;
    }
  }

  size_t getChunkSize(void) const { return mChunkSize; };

  bool isInside(const unsigned char *p) const
  {
      return p>=mData && p < mDataEnd;
  }

private:
  bool          mMyHeap;
  unsigned char         *mData;
  unsigned char         *mDataEnd;
  size_t         mUsedCount;
  MemoryHeader *mFreeList;
  size_t         mChunkSize;
};

#define DEFAULT_CHUNKS 32

class MemoryChunkChunk
{
public:
  MemoryChunkChunk(void)
  {
    mNext = 0;
    mChunkSize = 0;
    mMaxChunks = 0;
  }

  ~MemoryChunkChunk(void)
  {
  }

  inline void * allocate(MemoryChunk *&current,MicroChunkUpdate *update)
  {
    void *ret = 0;

    MemoryChunkChunk *scan = this;
    while ( scan && ret == 0 )
    {
      for (size_t i=0; i<DEFAULT_CHUNKS; i++)
      {
          ret = scan->mChunks[i].allocate(mHeap,mChunkSize,mMaxChunks,update);
          if ( ret )
          {
            current = &scan->mChunks[i];
            scan = 0;
            break;
          }
      }
      if ( scan )
        scan = scan->mNext;
    }

    if ( !ret )
    {
        MemoryChunkChunk *mcc = (MemoryChunkChunk *)mHeap->micro_malloc( sizeof(MemoryChunkChunk) );
        new ( mcc ) MemoryChunkChunk;
        MemoryChunkChunk *onext = mNext;
        mNext = mcc;
        mcc->mNext = onext;
        ret = mcc->mChunks[0].allocate(mHeap,mChunkSize,mMaxChunks,update);
        current = &mcc->mChunks[0];
    }

    return ret;
  }

  unsigned char *init(unsigned char *chunkBase,size_t fixedSize,size_t chunkSize,MemoryChunk *&current,MicroHeap *heap)
  {
    mHeap = heap;
    mChunkSize = chunkSize;
    mMaxChunks = fixedSize/chunkSize;
    current = &mChunks[0];
    chunkBase = mChunks[0].init(chunkBase,chunkSize,mMaxChunks);
    return chunkBase;
  }

  MicroHeap        *mHeap;
  size_t             mChunkSize;
  size_t             mMaxChunks;
  MemoryChunkChunk *mNext;
  MemoryChunk       mChunks[DEFAULT_CHUNKS];
};

class FixedMemory
{
public:
  FixedMemory(void)
  {
    mCurrent = 0;
  }

  void * allocate(MicroChunkUpdate *update)
  {
    void *ret = mCurrent->allocate(mChunks.mHeap,mChunks.mChunkSize,mChunks.mMaxChunks,update);
    if ( ret == 0 )
    {
        ret = mChunks.allocate(mCurrent,update);
    }
    return ret;
  }

  unsigned char *init(unsigned char *chunkBase,size_t chunkSize,size_t fixedSize,MicroHeap *heap)
  {
    mMemBegin = chunkBase;
    mMemEnd   = chunkBase+fixedSize;
    mChunks.init(chunkBase,fixedSize,chunkSize,mCurrent,heap);
    return mMemEnd;
  }

  unsigned char            *mMemBegin;
  unsigned char            *mMemEnd;
  MemoryChunk     *mCurrent; // the current memory chunk we are operating in.
  MemoryChunkChunk mChunks;  // the collection of all memory chunks used.
};

class MicroChunk
{
public:

  void set(unsigned char *memStart,unsigned char *memEnd,MemoryChunk *mc)
  {
    mMemStart = memStart;
    mMemEnd   = memEnd;
    mChunk    = mc;
    mPad      = 0;
  }

  inline bool inside(const unsigned char *p) const
  {
    return p >= mMemStart && p < mMemEnd;
  }

  unsigned char        *mMemStart;
  unsigned char        *mMemEnd;
  MemoryChunk *mChunk;
  unsigned char        *mPad; // padding to make it 16 byte aligned.
};

class MyMicroAllocator : public MicroAllocator, public MicroChunkUpdate, public MemMutex
{
public:
  MyMicroAllocator(MicroHeap *heap,void *baseMem,size_t initialSize,size_t chunkSize)
  {
    mLastMicroChunk     = 0;
    mMicroChunks        = 0;
    mMicroChunkCount    = 0;
    mMaxMicroChunks     = 0;
    mHeap               = heap;
    mChunkSize          = chunkSize;

    mBaseMem = (unsigned char *)baseMem;
    mBaseMemEnd = mBaseMem+initialSize;

    unsigned char *chunkBase = (unsigned char *)baseMem+sizeof(MyMicroAllocator);


    chunkBase+=32;
    size64 ptr = (size64)chunkBase;
    ptr = ptr>>4;
    ptr = ptr<<4; // make sure it is 16 byte aligned.
    chunkBase = (unsigned char *)ptr;

    // 0 through 512 bytes
	#define $fixed_allocator(N,from,to) do { \
		for( size_t i = from; i <= to; ++i ) { \
			mFixedAllocators[i] = &mAlloc[N]; \
		} \
		chunkBase = mAlloc[N].init(chunkBase,to,chunkSize,heap); \
	} while( 0 )

    mChunkStart = chunkBase;

    $fixed_allocator(  0,  0,  8 );
    $fixed_allocator(  1,  9, 16 );
    $fixed_allocator(  2, 17, 24 );
    $fixed_allocator(  3, 25, 32 );

    $fixed_allocator(  4, 33, 48 );
    $fixed_allocator(  5, 49, 64 );
    $fixed_allocator(  6, 65, 80 );
    $fixed_allocator(  7, 81, 96 );

    $fixed_allocator(  8, 97,128 );
    $fixed_allocator(  9,129,160 );
    $fixed_allocator( 10,161,192 );
    $fixed_allocator( 11,193,224 );

    $fixed_allocator( 12,225,296 );
    $fixed_allocator( 13,297,368 );
    $fixed_allocator( 14,369,440 );
    $fixed_allocator( 15,441,512 );
    $fixed_allocator( 16,513,1024 );

    assert( 16 + 1 == MAX_ALLOCATORS );

    mChunkEnd = chunkBase;

    assert(chunkBase <= mBaseMemEnd );
  }

  ~MyMicroAllocator(void)
  {
    if ( mMicroChunks )
    {
        mHeap->micro_free(mMicroChunks);
    }
  }

  virtual size_t           getChunkSize(MemoryChunk *chunk)
  {
      return chunk ? chunk->getChunkSize() : 0;
  }

  // we have to steal one byte out of every allocation to record the size, so we can efficiently de-allocate it later.
  virtual void * malloc(size_t size)
  {
    void *ret = 0;
    Lock();
   assert( size <= MAX_ALLOCATION_SIZE );
   if ( size <= MAX_ALLOCATION_SIZE )
   {
     ret = mFixedAllocators[size]->allocate(this);
   }
   Unlock();
    return ret;
  }

  virtual void   free(void *p,MemoryChunk *chunk)
  {
    Lock();
    chunk->deallocate(p,mHeap,this);
    Unlock();
  }

  // perform a binary search on the sorted list of chunks.
  MemoryChunk * binarySearchMicroChunks(const unsigned char *p)
  {
      MemoryChunk *ret = 0;

      size_t low = 0;
      size_t high = mMicroChunkCount;

      while ( low != high )
      {
          size_t mid = (high-low)/2+low;
          MicroChunk &chunk = mMicroChunks[mid];
          if ( chunk.inside(p))
          {
              mLastMicroChunk = &chunk;
              ret = chunk.mChunk;
              break;
          }
          else
          {
              if ( p > chunk.mMemEnd )
              {
                  low = mid+1;
              }
              else
              {
                  high = mid;
              }
          }
      }

      return ret;
  }

  virtual MemoryChunk *   isMicroAlloc(const void *p)  // returns true if this pointer is handled by the micro-allocator.
  {
    MemoryChunk *ret = 0;

    Lock();

    const unsigned char *s = (const unsigned char *)p;

    if ( s >= mChunkStart && s < mChunkEnd )
    {
        size_t index = (size_t)(s-mChunkStart)/mChunkSize;
        assert( index < MAX_ALLOCATORS );
        ret = &mAlloc[index].mChunks.mChunks[0];
        assert( ret->isInside(s) );
    }
    else if ( mMicroChunkCount )
    {
        if ( mLastMicroChunk && mLastMicroChunk->inside(s) )
        {
            ret = mLastMicroChunk->mChunk;
        }
        else
        {
            if ( mMicroChunkCount >= 4 )
            {
                ret = binarySearchMicroChunks(s);
$debug(
                if (ret )
                {
                    assert( ret->isInside(s) );
                }
                else
                {
                    for (size_t i=0; i<mMicroChunkCount; i++)
                    {
                        assert( !mMicroChunks[i].inside(s) );
                    }
                }
)
            }
            else
            {
                for (size_t i=0; i<mMicroChunkCount; i++)
                {
                    if ( mMicroChunks[i].inside(s) )
                    {
                        ret = mMicroChunks[i].mChunk;
                        assert( ret->isInside(s) );
                        mLastMicroChunk = &mMicroChunks[i];
                        break;
                    }
                }
            }
        }
    }
$debug(
    if ( ret )
        assert( ret->isInside(s) );
)
    Unlock();
    return ret;
  }

  MicroHeap * getMicroHeap(void) const { return mHeap; };

  void allocateMicroChunks(void)
  {
    if ( mMaxMicroChunks == 0 )
    {
        mMaxMicroChunks = 64; // initial reserve.
        mMicroChunks = (MicroChunk *)mHeap->micro_malloc( sizeof(MicroChunk)*mMaxMicroChunks );
    }
    else
    {
        mMaxMicroChunks*=2;
        mMicroChunks = (MicroChunk *)mHeap->micro_realloc( mMicroChunks, sizeof(MicroChunk)*mMaxMicroChunks);
    }
  }

  // perform an insertion sort of the new chunk.
  virtual void addMicroChunk(unsigned char *memStart,unsigned char *memEnd,MemoryChunk *chunk)
  {
    if ( mMicroChunkCount >= mMaxMicroChunks )
    {
        allocateMicroChunks();
    }

    bool inserted = false;
    for (size_t i=0; i<mMicroChunkCount; i++)
    {
        if ( memEnd < mMicroChunks[i].mMemStart )
        {
            for (size_t j=mMicroChunkCount; j>i; j--)
            {
                mMicroChunks[j] = mMicroChunks[j-1];
            }
            mMicroChunks[i].set( memStart, memEnd, chunk );
            mLastMicroChunk = &mMicroChunks[i];
            mMicroChunkCount++;
            inserted = true;
            break;
        }
    }
    if ( !inserted )
    {
        mMicroChunks[mMicroChunkCount].set(memStart,memEnd,chunk);
        mLastMicroChunk = &mMicroChunks[mMicroChunkCount];
        mMicroChunkCount++;
    }
  }

  virtual void removeMicroChunk(MemoryChunk *chunk)
  {
    mLastMicroChunk = 0;
    $debug(
    bool removed = false;
    )
        for (size_t i=0; i<mMicroChunkCount; i++)
    {
        if ( mMicroChunks[i].mChunk == chunk )
        {
            mMicroChunkCount--;
            for (size_t j=i; j<mMicroChunkCount; j++)
            {
                mMicroChunks[j] = mMicroChunks[j+1];
            }
            $debug(
            removed = true;
            )
                        break;
        }
    }
$debug(
    assert(removed);
)
  }

  inline void * inline_malloc(size_t size)
  {
     Lock();
     void *ret = mFixedAllocators[size]->allocate(this);
     Unlock();
     return ret;
  }

  inline void            inline_free(void *p,MemoryChunk *chunk) // free relative to previously located MemoryChunk
  {
    Lock();
    chunk->deallocate(p,mHeap,this);
    Unlock();
  }

  inline MemoryChunk *   inline_isMicroAlloc(const void *p) // returns pointer to the chunk this memory belongs to, or null if not a micro-allocated block.
  {
    MemoryChunk *ret = 0;

    Lock();

    const unsigned char *s = (const unsigned char *)p;

    if ( s >= mChunkStart && s < mChunkEnd )
    {
        size_t index = (size_t)(s-mChunkStart)/mChunkSize;
        assert( index < MAX_ALLOCATORS );
        ret = &mAlloc[index].mChunks.mChunks[0];
    }
    else if ( mMicroChunkCount )
    {
        if ( mLastMicroChunk && mLastMicroChunk->inside(s) )
        {
            ret = mLastMicroChunk->mChunk;
        }
        else
        {
            if ( mMicroChunkCount >= 4 )
            {
                ret = binarySearchMicroChunks(s);
            }
            else
            {
                for (size_t i=0; i<mMicroChunkCount; i++)
                {
                    if ( mMicroChunks[i].inside(s) )
                    {
                        ret = mMicroChunks[i].mChunk;
                        mLastMicroChunk = &mMicroChunks[i];
                        break;
                    }
                }
            }
        }
    }

    Unlock();

    return ret;
  }


private:
  MicroHeap   *mHeap;
  unsigned char        *mBaseMem;
  unsigned char        *mBaseMemEnd;
  FixedMemory *mFixedAllocators[MAX_ALLOCATION_SIZE+1];
  size_t        mChunkSize;
  unsigned char        *mChunkStart;
  unsigned char        *mChunkEnd;

  size_t        mMaxMicroChunks;
  size_t        mMicroChunkCount;
  MicroChunk  *mLastMicroChunk;
  MicroChunk  *mMicroChunks;

  FixedMemory  mAlloc[MAX_ALLOCATORS];
};


MicroAllocator *createMicroAllocator(MicroHeap *heap,size_t chunkSize)
{
    size_t initialSize = chunkSize*MAX_ALLOCATORS+sizeof(MyMicroAllocator)+32;
    void *baseMem = heap->micro_malloc(initialSize);
    MyMicroAllocator *mc = (MyMicroAllocator *)baseMem;
    new ( mc ) MyMicroAllocator(heap,baseMem,initialSize,chunkSize);
    return static_cast< MicroAllocator *>(mc);
}

void releaseMicroAllocator(MicroAllocator *m)
{
    MyMicroAllocator *mc = static_cast< MyMicroAllocator *>(m);
    MicroHeap *mh = mc->getMicroHeap();
    mc->~MyMicroAllocator();
    mh->micro_free(mc);
}

class MyHeapManager : public MicroHeap, public HeapManager
{
public:
  MyHeapManager(size_t defaultChunkSize)
  {
    mMicro = createMicroAllocator(this,defaultChunkSize);
  }

  ~MyHeapManager(void)
  {
    releaseMicroAllocator(mMicro);
  }

  // heap allocations used by the micro allocator.
  virtual void * micro_malloc(size_t size)
  {
	  void *ptr = mc_malloc( size );
	  assert( ptr );
    return ptr;
  }

  virtual void micro_free(void *p)
  {
    mc_free(p);
  }

  virtual void * micro_realloc(void *oldMem,size_t newSize)
  {
	  void *ptr = mc_realloc(oldMem,newSize);
	  assert( ptr );
    return ptr;
  }

  virtual void * heap_malloc(size_t size)
  {
    void *ret;

    if ( size <= MAX_ALLOCATION_SIZE ) // micro allocator only handles allocations between 0 and MAX_ALLOCATION_SIZE bytes in length.
    {
        ret = mMicro->malloc(size);
    }
    else
    {
        ret = mc_malloc(size);
		assert( ret );
    }
    return ret;
  }

  virtual void   heap_free(void *p)
  {
    MemoryChunk *chunk = mMicro->isMicroAlloc(p);
    if ( chunk )
    {
        mMicro->free(p,chunk);
    }
    else
    {
        mc_free(p);
    }
  }

  virtual void * heap_realloc(void *oldMem,size_t newSize)
  {
    void *ret = 0;

    MemoryChunk *chunk = mMicro->isMicroAlloc(oldMem);
    if ( chunk )
    {
        size_t oldSize = chunk->getChunkSize();
        if ( newSize <= oldSize ) {
          ret = oldMem;
        } else {
          ret = heap_malloc(newSize);
          memcpy(ret,oldMem,oldSize);
          mMicro->free(oldMem,chunk);
        }
    }
    else
    {
        ret = mc_realloc(oldMem,newSize);
		assert( ret );
    }

    return ret;
  }

  inline void * inline_heap_malloc(size_t size)
  {
    void *ptr = size<=MAX_ALLOCATION_SIZE ? ((MyMicroAllocator *)mMicro)->inline_malloc(size) : mc_malloc(size);
		assert( ptr );
  return memset( ptr, 0, size );
  }

  inline void   inline_heap_free(void *p)
  {
      MemoryChunk *chunk = ((MyMicroAllocator *)mMicro)->inline_isMicroAlloc(p);
      if ( chunk )
      {
          ((MyMicroAllocator *)mMicro)->inline_free(p,chunk);
      }
      else
      {
          mc_free(p);
      }

  }

private:
  MicroAllocator *mMicro;
};


HeapManager * createHeapManager(size_t defaultChunkSize)
{
    MyHeapManager *m = (MyHeapManager *)mc_malloc(sizeof(MyHeapManager));
    new ( m ) MyHeapManager(defaultChunkSize);
    return static_cast< HeapManager *>(m);
}

void          releaseHeapManager(HeapManager *heap)
{
    MyHeapManager *m = static_cast< MyHeapManager *>(heap);
    m->~MyHeapManager();
}

void * heap_malloc(HeapManager *hm,size_t size)
{
    return ((MyHeapManager *)hm)->inline_heap_malloc(size);
}

void   heap_free(HeapManager *hm,void *p)
{
    ((MyHeapManager *)hm)->inline_heap_free(p);
}

void * heap_realloc(HeapManager *hm,void *oldMem,size_t newSize)
{
    return hm->heap_realloc(oldMem,newSize);
}

}; // end of namespace

// tests

#define TEST_SIZE 63
#define TEST_ALLOC_COUNT 8192
#define TEST_RUN 40000000
#define TEST_INLINE 1

#ifndef _WIN32
static size_t timeGetTime(void)
{
    return 0;
}
#endif

static void performUnitTests(void)
{
    using namespace MICRO_ALLOCATOR;

    void *allocs[TEST_ALLOC_COUNT];
    for (size_t i=0; i<TEST_ALLOC_COUNT; i++)
    {
        allocs[i] = 0;
    }


    HeapManager *hm = createHeapManager(65536*32);


    {
      size_t stime = timeGetTime();
      srand(0);


      for (size_t i=0; i<TEST_RUN; i++)
      {
          size_t index = rand()&(TEST_ALLOC_COUNT-1);
          if ( allocs[index] )
          {
#if TEST_INLINE
              heap_free(hm, allocs[index] );
#else
              hm->heap_free( allocs[index] );
#endif
              allocs[index] = 0;
          }
          else
          {
              size_t asize = (rand()&TEST_SIZE);
              if ( (rand()&127)==0) asize+=MAX_ALLOCATION_SIZE; // one out of every 15 allocs is larger than MAX_ALLOCATION_SIZE bytes.
#if TEST_INLINE
              allocs[index] = heap_malloc(hm,asize);
#else
              allocs[index] = hm->heap_malloc(asize);
#endif
          }
      }

      for (size_t i=0; i<TEST_ALLOC_COUNT; i++)
      {
          if ( allocs[i] )
          {
#if TEST_INLINE
              heap_free(hm,allocs[i] );
#else
              hm->heap_free(allocs[i] );
#endif
              allocs[i] = 0;
          }
      }

      size_t etime = timeGetTime();
      printf("Micro allocation test took %d milliseconds.\r\n", etime - stime );

    }

    {

      size_t stime = timeGetTime();
      srand(0);


      for (size_t i=0; i<TEST_RUN; i++)
      {
          size_t index = rand()&(TEST_ALLOC_COUNT-1);
          if ( allocs[index] )
          {
              mc_free( allocs[index] );
              allocs[index] = 0;
          }
          else
          {
              size_t asize = (rand()&TEST_SIZE);
              if ( (rand()&127)==0) asize+=MAX_ALLOCATION_SIZE; // one out of every 15 allocs is larger than MAX_ALLOCATION_SIZE bytes.
              allocs[index] = mc_malloc(asize);
          }
      }

      for (size_t i=0; i<TEST_ALLOC_COUNT; i++)
      {
          if ( allocs[i] )
          {
              mc_free(allocs[i] );
              allocs[i] = 0;
          }
      }

      size_t etime = timeGetTime();
      printf("Standard malloc/free test took %d milliseconds.\r\n", etime - stime );

    }

    releaseHeapManager(hm);
}

/*
int main() {
  performUnitTests();
}
*/