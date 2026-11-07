@echo off
if not exist "controls-src\build-output" mkdir "controls-src\build-output"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /LD /MT /O2 /W4 /EHsc controls-src\ModernControls.cpp /Fo:controls-src\ModernControls.obj /link /OUT:controls-src\build-output\ModernControls.asi /IMPLIB:controls-src\ModernControls.lib user32.lib dinput8.lib dxguid.lib

