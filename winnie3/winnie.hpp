/*
(C) _Winnie woowoo 'at' list.ru

memory can not be taken in a single thread, and give to another.
This can happen if you use a STL container in different threads.

flow at the completion of their work should call CleanThreadAllocatorState, otherwise memory allocated allocator
will not be available.

uses microsoft specific __ declspec (thread)
GCC version by nsf
portable version by r-lyeh

allocator adapted only for node
containers, type set, map, multiset, multimap, list.
container type string, vector it will work, but using it is pointless, it will not be faster
*/

#if   defined(__GNUC__)
#define $THREAD          __thread
#define $NOINLINE
#define $STATIC(...)     static __VA_ARGS__
#define $GCC(...)        __VA_ARGS__
#define $MSVC(...)
#elif defined(_MSC_VER)
#define $THREAD          __declspec(thread)
#define $NOINLINE        __declspec(noinline)
#define $STATIC(...)     __VA_ARGS__ static
#define $MSVC(...)       __VA_ARGS__
#define $GCC(...)
#endif

namespace Winnie
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
		enum { CHUNK_SIZE = 4096 };

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

		$THREAD SThreadAllocatorState *pThreadAllocatorStates;

		template <size_t SIZE>
		class CFreeListSizedAllocator
		{
		private:

			$STATIC($THREAD)
			SThreadAllocatorState ThreadAllocatorState;

			static void Cleaner()
			{
				SChunkHeader *pChunk = ThreadAllocatorState.m_pChunksHead;
				while (pChunk)
				{
					SChunkHeader *pNext = pChunk->m_pNext;
					delete[] (char*)pChunk;
					pChunk = pNext;
				}

				std::cout << SIZE <<' ';
			}

			$STATIC($NOINLINE) void NewChunk()
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

		template <size_t SIZE>
		SThreadAllocatorState $GCC($THREAD) CFreeListSizedAllocator<SIZE>::ThreadAllocatorState;

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

		template <size_t SIZE>
		class CSizedAllocator: public SelectType<
			SIZE <= USE_OPERATOR_NEW,
			CFreeListSizedAllocator<SIZE >= sizeof(FreeBlock) ? SIZE : sizeof(FreeBlock)>,
			COperatorNewSizedAllocator<SIZE> >
		{
		};

	} // namespace Detail

	void CleanThreadAllocatorState()
	{
		for (
			Detail::SThreadAllocatorState *pThreadState = Detail::pThreadAllocatorStates;
			pThreadState;
			pThreadState = pThreadState->m_pNext)
			pThreadState->Cleaner();
	};

	template <class T>
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

		template <class U>
		CFastPoolAllocator(const CFastPoolAllocator<U> &in_Other) {};

		CFastPoolAllocator() {}

		pointer address(reference in_X) const { return &in_X; }
		const_pointer address(const_reference in_X) const { return &in_X; }
		size_type max_size() const { return size_t(-1)/sizeof(T); }

		pointer allocate(size_type in_Count, void * = 0)
		{
			if (in_Count != 1)
				return (T*)new char[in_Count*sizeof(T)];
			return (T*)Detail::CSizedAllocator<sizeof(T)>::Allocate();
		}

		void deallocate(pointer p, size_type in_Count)
		{
			if (in_Count != 1)
			{
				delete[] (char*)p;
				return;
			}
			Detail::CSizedAllocator<sizeof(T)>::Deallocate(p);
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
			typedef CFastPoolAllocator<U> other;
		};
	};
} //namespace Winnie

