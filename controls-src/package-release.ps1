param([Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+\.\d+$')][string]$Version,
      [Parameter(Mandatory=$true)][string]$BuildRoot)
$ErrorActionPreference = 'Stop'
$notes = Join-Path $PSScriptRoot 'ReleaseNotes.txt'
if (-not (Test-Path -LiteralPath $notes) -or (Get-Content -LiteralPath $notes -TotalCount 1) -ne "Version: $Version") {
    throw 'Update ReleaseNotes.txt for this version before packaging. Publishing requires matching release notes.'
}
$installedRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$root = (Resolve-Path -LiteralPath $BuildRoot).Path.TrimEnd('\')
if ($root -eq $installedRoot.TrimEnd('\')) { throw 'Package a staged build, never the installed game. The launcher alone updates the installed game.' }
if ((Get-Item -LiteralPath $root).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'The staging folder must not be a link.' }
$release = Join-Path $installedRoot 'release'
New-Item -ItemType Directory -Path $release -Force | Out-Null
$zip = Join-Path $release "ToyStory2-remaster-$Version.zip"
if (Test-Path -LiteralPath $zip) { throw 'This release already exists. Increment the version rather than replacing a published build.' }
$files = @('toy2.exe','toy2.cfg','toy2.ico','toy2.hlp','DDraw.dll','D3DImm.dll','dsound.dll','dsound.ini','dgVoodoo.conf','dgVoodooCpl.exe','Toy Story 2 Launcher.exe','Toy Story 2 Co-op.exe','Toy Story 2 Updater.exe','Coop-build.sha256','version.txt','Multiplayer.txt','Modern-controls.txt','Escape-menu-fix.txt','Register-Game.ps1','NetworkSetup.ps1','scripts\ModernControls.asi','scripts\ModernControls.ini','scripts\ToyStory2Fix.asi','scripts\ToyStory2Fix.ini','scripts\CoopBridge.asi')
foreach ($folder in @('data','DiscData')) { $files += Get-ChildItem -LiteralPath (Join-Path $root $folder) -File -Recurse | Where-Object { $_.Extension -notin '.sav','.log','.err','.bak','.backup' } | ForEach-Object { $_.FullName.Substring($root.Length+1) } }
foreach ($relative in $files) { if ($relative -ne 'version.txt' -and -not (Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)) { throw "Missing runtime file: $relative" } }
foreach ($line in Get-Content -LiteralPath (Join-Path $root 'Coop-build.sha256')) { $pair=$line -split '  ',2; if ((Get-FileHash -LiteralPath (Join-Path $root $pair[1]) -Algorithm SHA256).Hash -ne $pair[0]) { throw "Co-op manifest mismatch: $($pair[1])" } }
Set-Content -LiteralPath (Join-Path $root 'version.txt') -Value $Version -Encoding ASCII
$manifestRelative='data\runtime-files.sha256'
$files=@($files | Where-Object { $_ -ne $manifestRelative })
$manifestLines=foreach($relative in $files) { (Get-FileHash -LiteralPath (Join-Path $root $relative) -Algorithm SHA256).Hash+'  '+$relative }
Set-Content -LiteralPath (Join-Path $root $manifestRelative) -Value $manifestLines -Encoding ASCII
$files += $manifestRelative
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip,[IO.Compression.ZipArchiveMode]::Create)
try { foreach ($relative in $files) { [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,(Join-Path $root $relative),$relative.Replace('\','/'),[IO.Compression.CompressionLevel]::Optimal) | Out-Null } } finally { $archive.Dispose() }
$testExe = Join-Path $env:TEMP ('Toy2-PackageTest-'+[guid]::NewGuid().ToString('N')+'.exe')
try {
    & 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe' /nologo /target:exe /platform:x86 /main:UpdaterPackageTest "/out:$testExe" /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll (Join-Path $PSScriptRoot 'Updater.cs') (Join-Path $PSScriptRoot 'UpdaterPackageTest.cs')
    if ($LASTEXITCODE -ne 0) { throw 'Package verifier compilation failed.' }
    & $testExe $zip $root
    if ($LASTEXITCODE -ne 0) { throw 'Package verification failed. Do not publish this archive.' }
} finally { if (Test-Path -LiteralPath $testExe) { Remove-Item -LiteralPath $testExe -Force } }
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
Set-Content -LiteralPath ($zip+'.sha256') -Value ($hash+'  '+[IO.Path]::GetFileName($zip)) -Encoding ASCII
Copy-Item -LiteralPath (Join-Path $root 'Toy Story 2 Launcher.exe') -Destination (Join-Path $release 'Toy.Story.2.Launcher.exe') -Force
foreach ($old in Get-ChildItem -LiteralPath $release -File) {
    if ($old.Name -match '^ToyStory2-remaster-(\d+\.\d+\.\d+\.\d+)\.zip(?:\.sha256)?$' -and [version]$Matches[1] -lt [version]$Version) {
        if ($old.DirectoryName -ne $release -or ($old.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unsafe release cleanup path.' }
        Remove-Item -LiteralPath $old.FullName -Force
    }
}
"Created $zip ($([math]::Round((Get-Item -LiteralPath $zip).Length/1MB,1)) MiB), $($files.Count) runtime files. No saves or development files included."


