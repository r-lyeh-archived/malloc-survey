test-allocators
===============

A few allocation benchmarks, and results below:

```lisp

+---------------+
| running tests |X
+---------------+X
 XXXXXXXXXXXXXXXXX

single: std::allocator 100325us
multi: std::allocator 253763us
owner: std::allocator 331534us
single: tslf::allocator 508733us
single: tslf0::allocator 755769us
single: jemalloc::allocator 730228us
multi: jemalloc::allocator 2850209us
owner: jemalloc::allocator 3258355us
single: winnie::allocator 100128us
single: FSBAllocator 953712us
multi: FSBAllocator 30778226us
owner: FSBAllocator 31274030us
single: FSBAllocator2 880657us
multi: FSBAllocator2 30934377us
owner: FSBAllocator2 31403325us
single: boost::pool_allocator 2581172us
single: boost::fast_pool_allocator 2139576us
multi: boost::fast_pool_allocator 29176874us
owner: boost::fast_pool_allocator 32665795us
single: Winnie::CFastPoolAllocator 56865us
multi: Winnie::CFastPoolAllocator 171784us
owner: Winnie::CFastPoolAllocator 229987us
single: threadalloc::allocator 158254us
multi: threadalloc::allocator 469624us
owner: threadalloc::allocator 693874us
single: micro::allocator 805715us
multi: micro::allocator 10762194us
owner: micro::allocator 11152170us
single: iron::allocator 1040385us
single: tav::allocator 130948us
multi: tav::allocator 346245us
owner: tav::allocator 431261us
single: lt::allocator 102828us
multi: lt::allocator 231603us
owner: lt::allocator 295225us
single: dl::allocator 420578us
multi: dl::allocator 2194603us
owner: dl::allocator 2443303us

+---------------------------------------------------------------------+
| comparison table (RELEASE) (MSC 180031101) Wed Jun 17 12:43:32 2015 |X
+---------------------------------------------------------------------+X
 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

                                               THS RSM SFD AVG
 1st) winnie::allocator                        [ ] [ ] [ ] 33376 us (x3.31 times faster)
 2nd) Winnie::CFastPoolAllocator               [x] [ ] [ ] 76662 us (x1.44 times faster)
 3rd) lt::allocator                            [x] [ ] [ ] 98408 us (x1.12 times faster)
 4th) std::allocator                           [x] [ ] [ ] 110511 us (performs similar to standard allocator)
 5th) tav::allocator                           [x] [x] [ ] 143753 us (x1.3 times slower)
 6th) tslf::allocator                          [ ] [ ] [ ] 169577 us (x1.53 times slower)
 7th) threadalloc::allocator                   [x] [x] [ ] 231291 us (x2.09 times slower)
 8th) tslf0::allocator                         [ ] [x] [ ] 251923 us (x2.28 times slower)
 9th) iron::allocator                          [ ] [ ] [ ] 346795 us (x3.14 times slower)
10th) dl::allocator                            [x] [ ] [ ] 814434 us (x7.37 times slower)
11st) boost::pool_allocator                    [ ] [ ] [ ] 860390 us (x7.79 times slower)
12nd) jemalloc::allocator                      [x] [x] [ ] 1086118 us (x9.83 times slower)
13rd) micro::allocator                         [x] [x] [ ] 3717390 us (x33.6 times slower)
14th) FSBAllocator                             [x] [ ] [ ] 10424676 us (x94.3 times slower)
15th) FSBAllocator2                            [x] [x] [ ] 10467775 us (x94.7 times slower)
16th) boost::fast_pool_allocator               [x] [ ] [ ] 10888598 us (x98.5 times slower)

THS: THREAD_SAFE: safe to use in multithreaded scenarios (on is better)
RSM: RESET_MEMORY: allocated contents are reset to zero (on is better)
SFD: SAFE_DELETE: deallocated pointers are reset to zero (on is better)
AVG: AVG_SPEED: average time for each benchmark (lower is better)
```

## Older tests

```lisp
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
