param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference = 'Stop'
$installedRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$projectRoot = (Resolve-Path -LiteralPath $Destination).Path
if ($projectRoot.TrimEnd('\') -eq $installedRoot.TrimEnd('\')) { throw 'Direct installation is disabled. Supply a separate staged game folder.' }
if ((Get-Item -LiteralPath $projectRoot).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Staging must not be a link.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CoopBridge.asi') -Destination (Join-Path $projectRoot 'scripts\CoopBridge.asi') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Toy Story 2 Co-op.exe') -Destination (Join-Path $projectRoot 'Toy Story 2 Co-op.exe') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'TEST-INSTRUCTIONS.txt') -Destination (Join-Path $projectRoot 'Multiplayer.txt') -Force
$verifiedFiles = @('toy2.exe','DDraw.dll','D3DImm.dll','dsound.dll','scripts\ModernControls.asi','scripts\ToyStory2Fix.asi','scripts\CoopBridge.asi','data\chars\buzz.anm')
$manifest = foreach ($file in $verifiedFiles) {
    (Get-FileHash -LiteralPath (Join-Path $projectRoot $file) -Algorithm SHA256).Hash + '  ' + $file
}
Set-Content -LiteralPath (Join-Path $projectRoot 'Coop-build.sha256') -Value $manifest -Encoding ASCII
Write-Output "Co-op staged in $projectRoot; existing assets reused. No game launched."

