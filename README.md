test-allocators
===============

A few allocation benchmarks, and results below:

```
+---------------+
| running tests |X
+---------------+X
 XXXXXXXXXXXXXXXXX

single: jemalloc::allocator 439
multi: jemalloc::allocator 1176
owner: jemalloc::allocator 1446
single: winnie::allocator 57
single: FSBAllocator 641
multi: FSBAllocator 12841
owner: FSBAllocator 13181
single: FSBAllocator2 654
multi: FSBAllocator2 11195
owner: FSBAllocator2 11510
single: std::allocator 415
multi: std::allocator 1078
owner: std::allocator 1284
single: boost::pool_allocator 1677
single: boost::fast_pool_allocator 1226
multi: boost::fast_pool_allocator 12659
owner: boost::fast_pool_allocator 15086
single: Winnie::CFastPoolAllocator 50
multi: Winnie::CFastPoolAllocator 103
owner: Winnie::CFastPoolAllocator 141
single: threadalloc::allocator 124
multi: threadalloc::allocator 364
owner: threadalloc::allocator 499
single: micro::allocator 465
multi: micro::allocator 4488
owner: micro::allocator 4732
single: iron::allocator 635
single: tav::allocator 100
multi: tav::allocator 206
owner: tav::allocator 265
single: lt::allocator 78
multi: lt::allocator 162
owner: lt::allocator 208

+---------------------------------------------------------------------+
| comparison table (RELEASE) (MSC 180030324) Mon Jan 26 12:46:21 2015 |X
+---------------------------------------------------------------------+X
 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

                                               THS RSM SFD AVG
 1st) winnie::allocator                        [ ] [ ] [ ] 19 ms (x22.5 times faster)
 2nd) Winnie::CFastPoolAllocator               [x] [ ] [ ] 47 ms (x9.11 times faster)
 3rd) lt::allocator                            [x] [ ] [ ] 69 ms (x6.17 times faster)
 4th) tav::allocator                           [x] [x] [ ] 88 ms (x4.85 times faster)
 5th) threadalloc::allocator                   [x] [x] [ ] 166 ms (x2.57 times faster)
 6th) iron::allocator                          [ ] [ ] [ ] 211 ms (x2.02 times faster)
 7th) std::allocator                           [x] [ ] [ ] 428 ms (performs similar to standard allocator)
 8th) jemalloc::allocator                      [x] [x] [ ] 482 ms (x1.13 times slower)
 9th) boost::pool_allocator                    [ ] [ ] [ ] 559 ms (x1.31 times slower)
10th) micro::allocator                         [x] [x] [ ] 1577 ms (x3.69 times slower)
11st) FSBAllocator2                            [x] [x] [ ] 3836 ms (x8.96 times slower)
12nd) FSBAllocator                             [x] [ ] [ ] 4393 ms (x10.3 times slower)
13rd) boost::fast_pool_allocator               [x] [ ] [ ] 5028 ms (x11.7 times slower)

THS: THREAD_SAFE: safe to use in multithreaded scenarios (on is better)
RSM: RESET_MEMORY: allocated contents are reset to zero (on is better)
SFD: SAFE_DELETE: deallocated pointers are reset to zero (on is better)
AVG: AVG_SPEED: average time for each benchmark (lower is better)
```
