$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $env:TEMP ('Toy2-CompatibilityTest-'+[guid]::NewGuid().ToString('N'))))
New-Item -ItemType Directory -Path $root | Out-Null
$exe=Join-Path $root 'toy2.exe'
$other=Join-Path $root 'unrelated.exe'
$key='HKCU:\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers'
try {
    Set-Content (Join-Path $root 'Probe.cs') 'class Probe { static int Main() { return 0; } }'
    & 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe' /nologo /platform:x86 "/out:$exe" "/win32manifest:$PSScriptRoot\toy2.manifest" (Join-Path $root 'Probe.cs')
    if($LASTEXITCODE -ne 0){throw 'Probe compilation failed'}
    $source=Get-Content (Join-Path $PSScriptRoot 'GameLaunch.cs') -Raw
    Add-Type -TypeDefinition ($source + ' public class CompatibilityTestAccess { public static System.Diagnostics.ProcessStartInfo Create(string root) { return GameLaunch.Create(root); } }')
    New-Item -Path $key -Force | Out-Null
    Set-ItemProperty -LiteralPath $key -Name $exe -Value '~ DWM8And16BitMitigation WIN98 RUNASADMIN'
    Set-ItemProperty -LiteralPath $key -Name $other -Value '~ WIN98 RUNASADMIN'
    $blocked=$false
    try { $info=New-Object System.Diagnostics.ProcessStartInfo($exe); $info.UseShellExecute=$false; $p=[Diagnostics.Process]::Start($info); $p.WaitForExit() }
    catch { $e=$_.Exception; while($e.InnerException){$e=$e.InnerException}; if($e.NativeErrorCode -ne 740){throw}; $blocked=$true }
    if(-not $blocked){throw 'Failed to reproduce error 740'}
    $info=[CompatibilityTestAccess]::Create($root)
    if((Get-ItemPropertyValue -LiteralPath $key -Name $exe) -ne '~ DWM8And16BitMitigation'){throw 'Display flags changed'}
    if((Get-ItemPropertyValue -LiteralPath $key -Name $other) -ne '~ WIN98 RUNASADMIN'){throw 'Unrelated entry changed'}
    $p=[Diagnostics.Process]::Start($info)
    if(-not $p.WaitForExit(10000) -or $p.ExitCode -ne 0){throw 'Repaired probe failed'}
    [void][CompatibilityTestAccess]::Create($root)
    Set-ItemProperty -LiteralPath $key -Name $exe -Value '~ WIN98 RUNASADMIN'
    [void][CompatibilityTestAccess]::Create($root)
    if((Get-Item -LiteralPath $key).GetValue($exe)) {throw 'Empty flags not removed'}
    Write-Output 'PASS: automatic repair removes conflicting flags and starts probe without elevation; preserved other settings; repeated repair; empty entry cleanup.'
} finally {
    Remove-ItemProperty -LiteralPath $key -Name $exe -ErrorAction SilentlyContinue
    Remove-ItemProperty -LiteralPath $key -Name $other -ErrorAction SilentlyContinue
    if((Split-Path $root -Parent) -ne [IO.Path]::GetFullPath($env:TEMP) -or (Split-Path $root -Leaf) -notlike 'Toy2-CompatibilityTest-*'){throw 'Unexpected cleanup path'}
    Remove-Item -LiteralPath $root -Recurse -Force
}





