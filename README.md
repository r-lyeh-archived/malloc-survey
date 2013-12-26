test-allocators
===============

a few allocation benchmarks

```

+---------------+
| running tests |X
+---------------+X
 XXXXXXXXXXXXXXXXX

single: jemalloc::allocator 446
multi: jemalloc::allocator 1223
owner: jemalloc::allocator 1493
single: winnie::allocator 62
single: FSBAllocator 650
multi: FSBAllocator 13614
owner: FSBAllocator 13958
single: FSBAllocator2 629
multi: FSBAllocator2 13118
owner: FSBAllocator2 13440
single: std::allocator 413
multi: std::allocator 1138
owner: std::allocator 1347
single: boost::pool_allocator 1722
single: boost::fast_pool_allocator 1246
multi: boost::fast_pool_allocator 12563
owner: boost::fast_pool_allocator 15208
single: Winnie::CFastPoolAllocator 52
multi: Winnie::CFastPoolAllocator 137
owner: Winnie::CFastPoolAllocator 180
single: threadalloc::allocator 127
multi: threadalloc::allocator 370
owner: threadalloc::allocator 522
single: micro::allocator 477
multi: micro::allocator 4293
owner: micro::allocator 4547
single: iron::allocator 625
single: tav::allocator 102
multi: tav::allocator 238
owner: tav::allocator 299
single: lt::allocator 78
multi: lt::allocator 161
owner: lt::allocator 210

+---------------------------------------------------------------------+
| comparison table (RELEASE) (MSC 180030324) Thu Jun  5 13:41:30 2014 |X
+---------------------------------------------------------------------+X
 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

                                               THS RSM SFD AVG
 1st) winnie::allocator                        [ ] [ ] [ ] 20 ms (x21.7 times faster)
 2nd) Winnie::CFastPoolAllocator               [x] [ ] [ ] 60 ms (x7.48 times faster)
 3rd) lt::allocator                            [x] [ ] [ ] 70 ms (x6.41 times faster)
 4th) tav::allocator                           [x] [x] [ ] 99 ms (x4.51 times faster)
 5th) threadalloc::allocator                   [x] [x] [ ] 174 ms (x2.58 times faster)
 6th) iron::allocator                          [ ] [ ] [ ] 208 ms (x2.16 times faster)
 7th) std::allocator                           [x] [ ] [ ] 449 ms (performs similar to standard allocator)
 8th) jemalloc::allocator                      [x] [x] [ ] 497 ms (x1.11 times slower)
 9th) boost::pool_allocator                    [ ] [ ] [ ] 574 ms (x1.28 times slower)
10th) micro::allocator                         [x] [x] [ ] 1515 ms (x3.38 times slower)
11st) FSBAllocator2                            [x] [x] [ ] 4480 ms (x9.98 times slower)
12nd) FSBAllocator                             [x] [ ] [ ] 4652 ms (x10.4 times slower)
13rd) boost::fast_pool_allocator               [x] [ ] [ ] 5069 ms (x11.3 times slower)

THS: THREAD_SAFE: safe to use in multithreaded scenarios (on is better)
RSM: RESET_MEMORY: allocated contents are reset to zero (on is better)
SFD: SAFE_DELETE: deallocated pointers are reset to zero (on is better)
AVG: AVG_SPEED: average time for each benchmark (lower is better)
```
