@echo off

setlocal

if exist "%VS90COMNTOOLS%vsvars32.bat" (
    call "%VS90COMNTOOLS%vsvars32.bat"
)else (
    call "%VS80COMNTOOLS%vsvars32.bat"
)

set CC=cl /nologo /W3 /c /wd4146 /wd4244 /DJEMALLOC_STATIC_BUILD /D_REENTRANT /Iinclude /Iinclude/msvc_compat /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0500
set LIBER=lib /nologo

if not exist lib mkdir lib

%CC% /MT /O2 src\*.c
%LIBER% /out:lib\jemalloc.lib *.obj
del *.obj

%CC% /DJEMALLOC_DEBUG /MTd /Od /Zi /Fd"lib\jemallocD.pdb" src\*.c
%LIBER% /out:lib\jemallocD.lib *.obj
del *.obj

endlocal

pause