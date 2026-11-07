@echo off
setlocal
set "coopCompiler=C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
"%coopCompiler%" /nologo /target:winexe /platform:x86 /optimize+ "/out:controls-src\coop\Toy Story 2 Co-op.exe" /reference:System.Drawing.dll /reference:System.Windows.Forms.dll controls-src\coop\CoopLauncher.cs controls-src\coop\LocalTestFiles.cs controls-src\coop\CoopPreferences.cs controls-src\coop\CoopSession.cs controls-src\coop\GameFrame.cs controls-src\coop\NativeBridge.cs controls-src\UpdateGate.cs controls-src\GameLaunch.cs


