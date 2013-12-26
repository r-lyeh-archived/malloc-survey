@echo @echo [DEBUG] > make.bat
@echo @cl %* %%* test.cpp /Fetest.exe /Zi /EHsc /Oy- /DDEBUG /MTd >> make.bat
@make
