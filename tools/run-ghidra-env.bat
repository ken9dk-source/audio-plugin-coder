@echo off
setlocal
set "JAVA_HOME=C:\Users\ken98\.jdk\jdk-21.0.11+10"
set "GH=C:\APC\y\tools\ghidra_ext\ghidra_12.1_PUBLIC"
rmdir /S /Q "C:\APC\y\tools\ghidra_projenv" 2>/dev/null
mkdir "C:\APC\y\tools\ghidra_projenv" 2>/dev/null
call "%GH%\support\analyzeHeadless.bat" "C:\APC\y\tools\ghidra_projenv" vaz -import "C:\APC\y\tools\Vaz2010Core.dll" -scriptPath "C:\APC\y\tools\ghidra_scripts" -postScript DecompAt.java "C:\APC\y\tools\vaz_env.c" 0x4dbd7c 0x4dfbf8 0x4a073c 0x4a0a68 0x4a0864 -deleteProject
echo GHIDRA_EXIT=%ERRORLEVEL%
