
// walloc_test.cpp : test for winnie_alloc library and new_delete_example. 

#include "stdafx.h"

//do not interleave output  from different threads.
CRITICAL_SECTION COUT_CRITICAL_SECTION;

unsigned Rand()
{
  static unsigned randx = 0;
  return ((randx*= 3141592621u)+= 1235);
}

template <size_t max_size, bool random, size_t num_blocks>
void Test1()
{

  clock_t time = clock();
  
  typedef char *pchar;
  pchar *array_of_pointers = new pchar[num_blocks];
  for (size_t n= 0; n<num_blocks; ++n)
    array_of_pointers[n] = 0;

  for (int i=0; i<5000000; ++i)
  {
    int index = Rand()%num_blocks;
    delete[] array_of_pointers[index];
    int size = random ? Rand()%max_size : max_size;
    array_of_pointers[index] = new char[size];  

    //for (int k=0; k<size; ++k)
    //  array_of_pointers[index][k] = '*';
  }
  for (size_t index=0; index<num_blocks; ++index) 
    delete[] array_of_pointers[index];

  delete[] array_of_pointers;

  clock_t time2= clock() - time;

  EnterCriticalSection(&COUT_CRITICAL_SECTION);
  
  if (random)
  {
    std::cout 
      <<"Random size allocations(0 - " 
      <<max_size <<" size, number of blocks - " << num_blocks <<"): ";
  }
  else
  {
     std::cout 
       <<"Fixed size allocations(" 
       <<max_size <<" size, number of blocks - " << num_blocks <<"): ";
  }
  std::cout << 1000*double(time2) / CLOCKS_PER_SEC <<'\n'; 
  LeaveCriticalSection(&COUT_CRITICAL_SECTION);
}

#include <list>
#include <string>


void TestSTL()
{
  
  clock_t time = clock();
  for (int sample=0; sample<100000; ++sample)
  {
    const char *strings[8]=
    {
      "ab",
      "aab",
      "aaaab",
      "aaaaaaaab",
      "aaaaaaaaaaaaaaaab",
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab",
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab",
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab",
    };
    std::list<std::string> list;
    
    for (int i=0; i<10; ++i)
      list.push_back(strings[Rand() % 8]);

    for (
      std::list<std::string>::iterator 
        iter= list.begin(), 
        iter_end= list.end(); 
      iter != iter_end; 
      ++iter)
    {
      *iter = *iter + *iter;   
      *iter = "hi" + *iter;
    }

    
    std::list<std::string> list2 = list;
  
    assert(list2 == list);
  }

  clock_t time2= clock() - time;

  // MSDN says, cout operator << is likely not threadsafe:
  EnterCriticalSection(&COUT_CRITICAL_SECTION);
  std::cout <<"Test stl(note, this do not affect STLPort, which use it's own allocator): ";
  std::cout << 1000*double(time2) / CLOCKS_PER_SEC <<'\n'; 
  LeaveCriticalSection(&COUT_CRITICAL_SECTION);
}

void TestSTL2()
{
  
  clock_t time= clock();

  for (int sample = 0; sample < 10; ++sample)
  {
    std::list<std::list<char> > llc;

    for (int i=0; i<1000; ++i)
    {
      std::list<char> lc;
      for (int j=0; j<1000; ++j)
      {
        lc.push_back('w');
      }
      llc.push_back(lc);
    }
  }

  clock_t time2= clock() - time;
  std::cout <<"TestSTL2: " << time2 <<'\n';

};

void Test3()
{
  clock_t time = clock();
  for (int i=0; i<10000000; ++i)
  {
    char *px = new char[10];
    delete[] px;
  }
  clock_t time2= clock() - time;
  EnterCriticalSection(&COUT_CRITICAL_SECTION);
  std::cout <<"Simple new/delete loop: "<< time2 <<'\n';
  LeaveCriticalSection(&COUT_CRITICAL_SECTION);
}

#include "winnie_alloc.h"

void TestRealloc()
{
  int *p = (int*)Winnie::Alloc(0);

  for (int r=0; r<4000; ++r)
  {
    p = (int*)Winnie::Realloc(p,r*sizeof(int));
    if (r)
      p[r-1] = r;
#ifndef NDEBUG
    for (int i=0; i<r; ++i)
      assert(p[i] == (i+1));
#endif
  }
  for (int r=4000-1; r>=0; --r)
  {
    p = (int*)Winnie::Realloc(p,r*sizeof(int));
#ifndef NDEBUG
    for (int i=0; i<r; ++i)
      assert(p[i] == (i+1));
#endif
  }
  Winnie::Free(p);
}

#include "new_delete.h"

#include "winnie_alloc.h"

void Test(void *)
{
  TestRealloc();
  Test3();
  
  Test1<10, true, 100>(); 
  Test1<100, true, 100>();
  Test1<1000, true, 100>();
  Test1<10000, true, 100>();
  Test1<10, true, 100>();
  Test1<100, true, 100>();
  Test1<1000, true, 100>();
  Test1<10000, true, 100>();
  Test1<10, false, 100>();
  Test1<100, false, 100>();
  Test1<1000, false, 100>();
  Test1<10, false, 100>();
  Test1<100, false, 100>();
  Test1<1000, false, 100>();

  TestSTL();
  TestSTL();
  TestSTL2();
  TestSTL2();

  TestRealloc();
}

void TestMemoryLeaks()
{
  #if OP_NEW_DEBUG && OP_NEW_USE_WINNIE_ALLOC

  //op_new_break_alloc = 3;

  for (int i=0; i<3; ++i)
  {
    new char;
    char *pa = new char[2];
    
    if (i == 1)
      delete[] pa;

    new char[10];
  }

  std::cout 
    <<"testing memory leaks (note, STL and other libraries can take memory and never give it back):";
  
  for (
    void 
      *pum = OpNewBegin(), 
      *p_end = OpNewEnd();
    pum != p_end;
    pum = OpNewNextBlock(pum))
  {
    std::cout 
      <<"size: " <<OpNewGetHeader(pum)->size 
      <<", block # = {" <<(unsigned)OpNewGetHeader(pum)->block_num  <<"}\n";
  }

  OpNewValidateHeap(); //should be OK... if this is a lucky day.

  char *p = new char [100];

  OpNewAssertIsValid(p);
  OpNewAssertIsValid(OpNewGetBlockPointer(p+50));
  //OpNewAssertIsValid(p+50);    -   ERROR!!! 

  char *p2 = (char*)OpNewGetBlockPointer(p+50);
  assert(p2 == p);

  assert(OpNewGetHeader(p2)->size == 100);

#endif
}

int main(int, char*[])
{ 
#if (OP_NEW_USE_WINNIE_ALLOC && OP_NEW_MULTITHREADING)
  {
    OperatorNewCriticalSectionInit(); //Should be called before any concurrent allocations in other threads 
  }
#endif
  TestMemoryLeaks();
  
  InitializeCriticalSection(&COUT_CRITICAL_SECTION);

#if OP_NEW_MULTITHREADING 
  std::cout <<"Run allocation test in one thread:\n";
  Test(0);
  std::cout <<"testing 2 concurrent threads...\n";
  HANDLE h1 = (HANDLE)_beginthread(Test, 0, 0);
  HANDLE h2 = (HANDLE)_beginthread(Test, 0, 0);
  WaitForSingleObject(h1, INFINITE);
  WaitForSingleObject(h2, INFINITE);
  std::cout <<"2 concurrent threads stopped\n";

#else
  Test(0);
#endif

  Winnie::Statistic st; 
  Winnie::GetAllocInfo(st);

  return 0;
}
