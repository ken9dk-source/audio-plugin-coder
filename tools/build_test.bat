@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d C:\APC\y\tools
cl /nologo /O2 /std:c++17 /EHsc test_typec.cpp /Fe:test_typec.exe
if errorlevel 1 ( echo COMPILE_FAILED & exit /b 1 )
test_typec.exe
