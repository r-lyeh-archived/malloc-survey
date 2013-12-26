@echo @echo [RELEASE] > make.bat
@echo @cl %* %%* test.cpp /Fetest.exe /EHsc /Ox /Oy /DNDEBUG /MT >> make.bat
@make

