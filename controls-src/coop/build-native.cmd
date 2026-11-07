@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /LD /MT /O2 /W4 /EHsc controls-src\coop\CoopBridge.cpp /Fo:controls-src\coop\CoopBridge.obj /link /OUT:controls-src\coop\CoopBridge.asi /IMPLIB:controls-src\coop\CoopBridge.lib
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /W4 /EHsc controls-src\coop\NativeTests.cpp /Fo:controls-src\coop\NativeTests.obj /Fe:controls-src\coop\NativeTests.exe
if errorlevel 1 exit /b 1
controls-src\coop\NativeTests.exe
