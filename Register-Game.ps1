$ErrorActionPreference = 'Stop'
$base = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryView]::Registry32)
$key = $base.CreateSubKey('SOFTWARE\TravellersTalesToyStory2')
$key.SetValue('path',(Join-Path $PSScriptRoot 'data\'))
$key.SetValue('cdpath',(Join-Path $PSScriptRoot 'DiscData\'))
$key.Close()
$base.Close()
