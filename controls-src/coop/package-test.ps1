param([Parameter(Mandatory=$true)][string]$Destination)
& (Join-Path $PSScriptRoot 'install-main.ps1') -Destination $Destination
