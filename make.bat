@echo [RELEASE] 
@cl %* tlsf0\*.c* tlsf\*.c* ltalloc\*.c* dlmalloc\*.c* jemalloc-win32\src\*.c -I jemalloc-win32\include -DJEMALLOC_STATIC_BUILD -D_REENTRANT -I jemalloc-win32\include\msvc_compat %* -I boost_1_58_0 boost_1_58_0\stage\lib\*-s-*.lib %* test.cpp /Fetest.exe /EHsc /Ox /Oy /DNDEBUG /MT 
