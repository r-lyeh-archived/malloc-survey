malloc-survey
=============

A few allocation benchmarks, and results below:

```lisp
+---------------+
| running tests |X
+---------------+X
 XXXXXXXXXXXXXXXXX

single: std::allocator 91621us
multi: std::allocator 276511us
owner: std::allocator 353299us
single: tlsf::allocator 471562us
single: tlsf0::allocator 697469us
single: dumb_tlsf::allocator 124982us
single: jemalloc::allocator 705766us
multi: jemalloc::allocator 2413796us
owner: jemalloc::allocator 2797227us
single: winnie1::allocator 66307us
single: winnie2::allocator 97257us
single: FSBAllocator 86354us
single: FSBAllocator2 56512us
single: boost::pool_allocator 815094us
single: boost::fast_pool_allocator 403297us
single: winnie3::CFastPoolAllocator 56200us
multi: winnie3::CFastPoolAllocator 181906us
owner: winnie3::CFastPoolAllocator 247601us
single: threadalloc::allocator 157090us
multi: threadalloc::allocator 632310us
owner: threadalloc::allocator 856387us
single: microallocator::allocator 655286us
multi: microallocator::allocator 10379597us
owner: microallocator::allocator 10773399us
single: iron::allocator 592763us
single: tav::allocator 124635us
multi: tav::allocator 368583us
owner: tav::allocator 450269us
single: ltalloc::allocator 92549us
multi: ltalloc::allocator 271687us
owner: ltalloc::allocator 332669us
single: dlmalloc::allocator 142754us

+---------------------------------------------------------------------+
| comparison table (RELEASE) (MSC 190023419) Fri Mar 25 11:35:52 2016 |X
+---------------------------------------------------------------------+X
 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

                                               THS RSM SFD AVG
 1st) FSBAllocator2                            [ ] [ ] [ ] 18837 us (x6.25 times faster)
 2nd) winnie1::allocator                       [ ] [ ] [ ] 22102 us (x5.33 times faster)
 3rd) FSBAllocator                             [ ] [ ] [ ] 28784 us (x4.09 times faster)
 4th) winnie2::allocator                       [ ] [ ] [ ] 32419 us (x3.63 times faster)
 5th) dumb_tlsf::allocator                     [ ] [x] [ ] 41660 us (x2.83 times faster)
 6th) dlmalloc::allocator                      [ ] [ ] [ ] 47584 us (x2.47 times faster)
 7th) winnie3::CFastPoolAllocator              [x] [ ] [ ] 82533 us (x1.43 times faster)
 8th) ltalloc::allocator                       [x] [ ] [ ] 110889 us (x1.06 times faster)
 9th) std::allocator                           [x] [ ] [ ] 117766 us (performs similar to standard allocator)
10th) boost::fast_pool_allocator               [ ] [ ] [ ] 134432 us (x1.14 times slower)
11st) tav::allocator                           [x] [x] [ ] 150089 us (x1.27 times slower)
12nd) tlsf::allocator                          [ ] [ ] [ ] 157187 us (x1.33 times slower)
13rd) iron::allocator                          [ ] [ ] [ ] 197587 us (x1.68 times slower)
14th) tlsf0::allocator                         [ ] [x] [ ] 232489 us (x1.97 times slower)
15th) boost::pool_allocator                    [ ] [ ] [ ] 271698 us (x2.31 times slower)
16th) threadalloc::allocator                   [x] [x] [ ] 285462 us (x2.42 times slower)
17th) jemalloc::allocator                      [x] [x] [ ] 932409 us (x7.92 times slower)
18th) microallocator::allocator                [x] [x] [ ] 3591133 us (x30.5 times slower)

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

single: std::allocator 107670us
multi: std::allocator 263339us
owner: std::allocator 344551us
single: tlsf::allocator 505713us
single: tlsf0::allocator 760873us
single: dumb_tlsf::allocator 131292us
single: jemalloc::allocator 725814us
multi: jemalloc::allocator 2889605us
owner: jemalloc::allocator 3375072us
single: winnie::allocator 100762us
single: FSBAllocator 82423us
single: FSBAllocator2 57373us
single: boost::pool_allocator 2574193us
single: boost::fast_pool_allocator 2208098us
single: Winnie::CFastPoolAllocator 58541us
multi: Winnie::CFastPoolAllocator 174008us
owner: Winnie::CFastPoolAllocator 233962us
single: threadalloc::allocator 165063us
multi: threadalloc::allocator 571112us
owner: threadalloc::allocator 813553us
single: micro::allocator 665540us
multi: micro::allocator 10949118us
owner: micro::allocator 11382275us
single: iron::allocator 927141us
single: tav::allocator 130940us
multi: tav::allocator 383240us
owner: tav::allocator 466599us
single: lt::allocator 108499us
multi: lt::allocator 311984us
owner: lt::allocator 371317us
single: dl::allocator 419116us
multi: dl::allocator 2190576us
owner: dl::allocator 2424850us

+---------------------------------------------------------------------+
| comparison table (RELEASE) (MSC 180031101) Thu Jun 25 12:53:34 2015 |X
+---------------------------------------------------------------------+X
 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

                                               THS RSM SFD AVG
 1st) FSBAllocator2                            [ ] [ ] [ ] 19124 us (x6.01 times faster)
 2nd) FSBAllocator                             [ ] [ ] [ ] 27474 us (x4.18 times faster)
 3rd) winnie::allocator                        [ ] [ ] [ ] 33587 us (x3.42 times faster)
 4th) dumb_tlsf::allocator                     [ ] [x] [ ] 43764 us (x2.62 times faster)
 5th) Winnie::CFastPoolAllocator               [x] [ ] [ ] 77987 us (x1.47 times faster)
 6th) std::allocator                           [x] [ ] [ ] 114850 us (performs similar to standard allocator)
 7th) lt::allocator                            [x] [ ] [ ] 123772 us (x1.08 times slower)
 8th) tav::allocator                           [x] [x] [ ] 155533 us (x1.35 times slower)
 9th) tlsf::allocator                          [ ] [ ] [ ] 168571 us (x1.47 times slower)
10th) tlsf0::allocator                         [ ] [x] [ ] 253624 us (x2.21 times slower)
11st) threadalloc::allocator                   [x] [x] [ ] 271184 us (x2.36 times slower)
12nd) iron::allocator                          [ ] [ ] [ ] 309047 us (x2.69 times slower)
13rd) boost::fast_pool_allocator               [ ] [ ] [ ] 736032 us (x6.41 times slower)
14th) dl::allocator                            [x] [ ] [ ] 808283 us (x7.04 times slower)
15th) boost::pool_allocator                    [ ] [ ] [ ] 858064 us (x7.47 times slower)
16th) jemalloc::allocator                      [x] [x] [ ] 1125024 us (x9.8 times slower)
17th) micro::allocator                         [x] [x] [ ] 3794091 us (x33 times slower)

THS: THREAD_SAFE: safe to use in multithreaded scenarios (on is better)
RSM: RESET_MEMORY: allocated contents are reset to zero (on is better)
SFD: SAFE_DELETE: deallocated pointers are reset to zero (on is better)
AVG: AVG_SPEED: average time for each benchmark (lower is better)
```

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
