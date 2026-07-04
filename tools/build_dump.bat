@echo off
cd /d C:\APC\y\tools
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
cl /nologo /O2 /EHsc /Fe:vaz_coef_dump.exe vaz_coef_dump.cpp
echo BUILD_DONE_%ERRORLEVEL%
