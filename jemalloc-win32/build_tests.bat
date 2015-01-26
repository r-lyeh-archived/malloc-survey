@echo off

setlocal

if exist "%VS90COMNTOOLS%vsvars32.bat" (
    call "%VS90COMNTOOLS%vsvars32.bat"
) else (
    call "%VS80COMNTOOLS%vsvars32.bat"
)

set CC=cl /nologo /MT /O2 /W3 /c /wd4146 /wd4244 /DJEMALLOC_STATIC_BUILD /D_REENTRANT /Iinclude /Iinclude/msvc_compat /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0500
set LINKER=link /nologo  lib\jemalloc.lib

rem debug
rem set CC=cl /nologo /DJEMALLOC_DEBUG /MTd /Od /Zi /W3 /c /wd4146 /wd4244 /DJEMALLOC_STATIC_BUILD /D_REENTRANT /Iinclude /Iinclude/msvc_compat /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0500
rem set LINKER=link /nologo /DEBUG lib\jemallocD.lib

%CC% test\*.c

set file=aligned_alloc
%LINKER% /out:test\%file%.exe %file%.obj 

set file=allocated
%LINKER% /out:test\%file%.exe %file%.obj

set file=allocm
%LINKER% /out:test\%file%.exe %file%.obj

set file=bitmap
%LINKER% /out:test\%file%.exe %file%.obj

set file=mremap
%LINKER% /out:test\%file%.exe %file%.obj

set file=posix_memalign
%LINKER% /out:test\%file%.exe %file%.obj

set file=rallocm
%LINKER% /out:test\%file%.exe %file%.obj

set file=thread_arena
%LINKER% /out:test\%file%.exe %file%.obj

set file=thread_tcache_enabled
%LINKER% /out:test\%file%.exe %file%.obj

set file=ALLOCM_ARENA
%LINKER% /out:test\%file%.exe %file%.obj

del *.obj

pause 