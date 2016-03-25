cl *.cpp -I ..\ ..\*.cpp /Fealloc-enabled.exe -DOP_NEW_USE_WINNIE_ALLOC=1 /DNDEBUG /Ox /Oy /MT 
cl *.cpp -I ..\ ..\*.cpp /Fealloc-disabled.exe -DOP_NEW_USE_WINNIE_ALLOC=0  /DNDEBUG /Ox /Oy /MT
