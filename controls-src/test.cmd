@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /MT /O2 /W4 /EHsc controls-src\test_controls.cpp /Fo:controls-src\test_controls.obj /Fe:controls-src\test_controls.exe /link user32.lib dinput8.lib dxguid.lib
if errorlevel 1 exit /b 1
controls-src\test_controls.exe
