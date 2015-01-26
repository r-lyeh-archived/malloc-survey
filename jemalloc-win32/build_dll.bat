@echo off

setlocal

if exist "%VS90COMNTOOLS%vsvars32.bat" (
    call "%VS90COMNTOOLS%vsvars32.bat"
)else (
    call "%VS80COMNTOOLS%vsvars32.bat"
)

set CC=cl /nologo /W3 /c /wd4146 /wd4244 /D_REENTRANT /Iinclude /Iinclude/msvc_compat /DDLLEXPORT /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0500
set LINKER=link /nologo /DLL

if not exist dll mkdir dll

%CC% /MT /O2 src\*.c
%LINKER% /out:dll\jemalloc.dll *.obj
del *.obj

%CC% /DJEMALLOC_DEBUG /MTd /Od /Zi /Fd"dll\jemallocD.pdb" src\*.c
%LINKER% /out:dll\jemallocD.dll *.obj
del *.obj

endlocal

pause