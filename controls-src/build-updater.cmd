@echo off
if not exist "controls-src\build-output" mkdir "controls-src\build-output"
"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /platform:x86 /optimize+ "/out:controls-src\build-output\Toy Story 2 Updater.exe" /win32icon:toy2.ico /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll controls-src\Updater.cs

