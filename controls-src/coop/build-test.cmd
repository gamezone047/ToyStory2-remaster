@echo off
setlocal
set "coopCompiler=C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
"%coopCompiler%" /nologo /target:library /platform:x86 /optimize+ /out:controls-src\coop\Toy2Coop.Network.dll controls-src\coop\CoopPreferences.cs controls-src\coop\CoopSession.cs controls-src\coop\GameFrame.cs controls-src\coop\NativeBridge.cs
if errorlevel 1 exit /b 1
"%coopCompiler%" /nologo /target:exe /platform:x86 /optimize+ /out:controls-src\coop\CoopTests.exe controls-src\coop\CoopPreferences.cs controls-src\coop\CoopSession.cs controls-src\coop\GameFrame.cs controls-src\coop\NativeBridge.cs controls-src\coop\CoopTests.cs controls-src\coop\LocalTestFiles.cs controls-src\coop\LocalTestTests.cs
if errorlevel 1 exit /b 1
controls-src\coop\CoopTests.exe
