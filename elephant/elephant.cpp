// Elephant Memory Allocations
#include <stdio.h>
#include <JRSMemory_Pools.h>

using namespace Elephant;

struct manager {

	enum { s_defaultAlignment = 16 };
	cHeap *heap;

	static void MemoryManagerLogText(const jrs_i8 *pText) {
		printf("%s\n",pText);
	}

	static void MemoryManagerErrorHandle(const jrs_i8 *pError, jrs_u32 uErrorID) {
		int* pCrasher = 0;
	    *pCrasher = 42;
	}

	static void MemoryManagerWriteToFile(void *pData, int size, const jrs_i8 *pFilePathAndName, jrs_bool bAppend) {
		// Standard C file output. Note that it always closes the file and rarely creates a new one.
		FILE *fp;
		fp = fopen(pFilePathAndName, (bAppend) ? "ab" : "wb");
		if(fp)
		{
			fwrite(pData, size, 1, fp);
			fclose(fp);
		}
	}

	manager() : heap(0) {

		cMemoryManager &self = cMemoryManager::Get();

		self.InitializeCallbacks(MemoryManagerLogText, MemoryManagerErrorHandle, MemoryManagerWriteToFile);

		//do not clear memory when allocated and deleted using the following values (used a lot by jet/goa)
		cHeap::sHeapDetails doNotClearDetails;
		doNotClearDetails.bHeapClearing = false;
		doNotClearDetails.bAllowZeroSizeAllocations = true;
		doNotClearDetails.bAllowDestructionWithAllocations = true; //to set when wanting to clear a heap without deallocating everything

		//Heap sizes in MB
		const size_t k_TotalHeapSize = (size_t)(128 * 1.0);

		//memManager.InitializeEnhancedDebugging(true);
		bool init = self.Initialize(k_TotalHeapSize << 20, 0, false);

		jrs_u64 availableMem = self.GetFreeUsableMemory();

		heap = self.CreateHeap(availableMem, "MyMainHeap", &doNotClearDetails);
	}
	~manager() {
		cMemoryManager &self = cMemoryManager::Get();

		if (heap) {
			self.DestroyHeap(heap);
			heap = 0;
		}

		self.Destroy();
	}
	void *malloc( size_t sz ) {
		return heap->AllocateMemory(sz, s_defaultAlignment);
	}
	void free( void *ptr ) {
		heap->FreeMemory(ptr);
	}
};

namespace {
	manager &get() {
		static manager mgr;
		return mgr;
	}
}

void *elf_malloc( size_t sz ) {
	return get().malloc( sz );
}

void elf_free( void* ptr ) {
	return get().free( ptr );
}
