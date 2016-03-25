/*
(C) _Winnie woowoo 'at' list.ru
(C) 2009 MihaPro: template parameter passed to the chunk size

Memory can not be taken in a single thread, and give to another.
This can happen if you use a STL container in different threads.

The thread at the end of their work should call CleanThreadAllocatorState, 
otherwise allocated memory by allocator will not be released.

using microsoft specific __declspec (thread)

Allocator is adapted only for nodes containers such as set, map, multiset, multimap, list.
For vector, string type containers it will work, but pointless, he will not ??? std::allocator
*/

#if     defined(__GNUC__)
#define STORAGE          __thread
#elif   defined(_MSC_VER)
#define STORAGE          __declspec(thread)
#else
#define STORAGE
#endif


namespace MWinnie
{

  namespace Detail
  {
    template <bool, class T1, class T2>
    struct SelectType: public T1
    {
    };

    template <class T1, class T2>
    struct SelectType<false, T1, T2> : public T2
    {
    };

    enum { USE_OPERATOR_NEW = 512 };

    struct SChunkHeader
    {
      SChunkHeader *m_pNext;
    };

    struct FreeBlock
    {
      FreeBlock *m_pNext;
    };

    struct SThreadAllocatorState
    {
      SThreadAllocatorState *m_pNext;
      void (*Cleaner)();
      SChunkHeader *m_pChunksHead;
      FreeBlock *m_pFirstFree;
    };

    STORAGE SThreadAllocatorState *pThreadAllocatorStates;


    //STATIC_ASSERT (???? ?????? ?????????????)_____________________________
    template<bool>
    struct CompileTimeChecker
    {
      CompileTimeChecker(...);
      operator int();
    };
    template<> struct CompileTimeChecker<false> { operator int(); };

#define STATIC_ASSERT(expr, msg)                      \
    {                                      \
    class ERROR_##msg {};                          \
    (void)sizeof( ( int )CompileTimeChecker<(expr) != 0>((ERROR_##msg())));  \
    }


    template <size_t SIZE, size_t CHUNK_SIZE>
    class CFreeListSizedAllocator
    {

    private:

      STORAGE
        static SThreadAllocatorState ThreadAllocatorState;

      static void Cleaner()
      {
        SChunkHeader *pChunk = ThreadAllocatorState.m_pChunksHead;
        while (pChunk)
        {
          SChunkHeader *pNext = pChunk->m_pNext;
          delete[] (char*)pChunk;
          pChunk = pNext;
        }

        //std::cout << SIZE <<' ';
      }

      static __declspec(noinline) void NewChunk()
      {
        if (!ThreadAllocatorState.m_pChunksHead)
        {
          //????????? ???? ? ??????????? ?????? ???????????, ??????? ????? ??????? Clean ??? ?????? ?? ??????.
          ThreadAllocatorState.m_pNext = pThreadAllocatorStates;
          ThreadAllocatorState.Cleaner = Cleaner;
          pThreadAllocatorStates = &ThreadAllocatorState;
        }
        SChunkHeader *pNewChunk = (SChunkHeader*)new char[CHUNK_SIZE+sizeof(SChunkHeader)];
        pNewChunk->m_pNext = ThreadAllocatorState.m_pChunksHead;
        ThreadAllocatorState.m_pChunksHead = pNewChunk;
        char *pChunkStorage = (char *)(pNewChunk+1);

        // ???? ????? ?? ??????????, ?????? ?? ??????? ??????? ????? ??? ???????? ??????? 1 ????????
        STATIC_ASSERT((CHUNK_SIZE + sizeof(SChunkHeader)) > SIZE, CHECK_CHUNK_SIZE_FAILED);

        const size_t NumBlocksInChunk = CHUNK_SIZE/SIZE - 1;
        for (int i=NumBlocksInChunk; i >= 0 ; --i)
        {
          FreeBlock *pFree = (FreeBlock*)(pChunkStorage + i*SIZE);
          pFree->m_pNext = (FreeBlock*)ThreadAllocatorState.m_pFirstFree;
          ThreadAllocatorState.m_pFirstFree = pFree;
        }
      }

    public:

      static void *Allocate()
      {
        if (!ThreadAllocatorState.m_pFirstFree)
          NewChunk();
        FreeBlock *pFree = ThreadAllocatorState.m_pFirstFree;
        ThreadAllocatorState.m_pFirstFree = pFree->m_pNext;
        return pFree;
      }

      static void Deallocate(void *p)
      {
        FreeBlock *pFree = (FreeBlock*)p;
        pFree->m_pNext =  ThreadAllocatorState.m_pFirstFree;
        ThreadAllocatorState.m_pFirstFree = pFree;
      }
    };

#undef STATIC_ASSERT                      \

    template <size_t SIZE, size_t CHUNK_SIZE>
    SThreadAllocatorState CFreeListSizedAllocator<SIZE, CHUNK_SIZE>::ThreadAllocatorState;

    template <size_t SIZE>
    class COperatorNewSizedAllocator
    {
    public:
      static void *Allocate()
      {
        return new char[SIZE];
      }

      static void Deallocate(void *p)
      {
        delete[] (char*)p;
      }
    };

    template <size_t SIZE, size_t CHUNK_SIZE>
    class CSizedAllocator: public SelectType<
      SIZE <= USE_OPERATOR_NEW,
      CFreeListSizedAllocator<SIZE >= sizeof(FreeBlock) ? SIZE : sizeof(FreeBlock), CHUNK_SIZE>,
      COperatorNewSizedAllocator<SIZE> >
    {
    };

  }// namespace Detail

  void CleanThreadAllocatorState()
  {
    for (
      Detail::SThreadAllocatorState *pThreadState = Detail::pThreadAllocatorStates;
      pThreadState;
    pThreadState = pThreadState->m_pNext)
      pThreadState->Cleaner();
  };

  // MihaPro: 4092 = ??? 4K ?????? ??? 4? ???? ??? ?????????
  template <class T, size_t CHUNK_SIZE = 4092>
  class CFastPoolAllocator
  {
  public:
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef size_t size_type;
    typedef T value_type;

    template <class U, size_t CHUNK_SIZE>
    CFastPoolAllocator(const CFastPoolAllocator<U, CHUNK_SIZE> &in_Other) {};

    CFastPoolAllocator() {}

    pointer address(reference in_X) const { return &in_X; }
    const_pointer address(const_reference in_X) const { return &in_X; }
    size_type max_size() const { return size_t(-1)/sizeof(T); }

    pointer allocate(size_type in_Count, void * = 0)
    {
      if (in_Count != 1)
        return (T*)new char[in_Count*sizeof(T)];
      return (T*)Detail::CSizedAllocator<sizeof(T), CHUNK_SIZE>::Allocate();
    }

    void deallocate(pointer p, size_type in_Count)
    {
      if (in_Count != 1)
      {
        delete[] (char*)p;
        return;
      }
      Detail::CSizedAllocator<sizeof(T), CHUNK_SIZE>::Deallocate(p);
    }

    void construct(pointer out_p, const value_type &in_Val)
    {
      new (out_p)T(in_Val);
    }

    void destroy(pointer p)
    {
      p->~T();
    }

    template <class U>
    struct rebind
    {
      typedef CFastPoolAllocator<U, CHUNK_SIZE> other;
    };
  };

} //namespace MWinnie
