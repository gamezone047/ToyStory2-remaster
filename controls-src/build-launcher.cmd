@echo off
if not exist "controls-src\build-output" mkdir "controls-src\build-output"
"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /main:Launcher /platform:x86 /optimize+ "/out:controls-src\build-output\Toy Story 2 Launcher.exe" /win32icon:toy2.ico /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll controls-src\Launcher.cs controls-src\UpdateGate.cs controls-src\GameLaunch.cs controls-src\Updater.cs controls-src\PortableInstall.cs controls-src\LauncherSkin.cs /resource:controls-src\launcher-art\title.png,Launcher.Title /resource:controls-src\launcher-art\font.png,Launcher.Font




