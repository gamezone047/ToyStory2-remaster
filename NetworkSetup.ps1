param([switch]$RemoveMapping,[string]$ReportPath)
$ErrorActionPreference = 'Stop'
if ($ReportPath) { Start-Transcript -LiteralPath $ReportPath -Force | Out-Null }
$port = 27922
$description = 'Toy Story 2 Co-op'
$route = Get-NetRoute -DestinationPrefix '0.0.0.0/0' | Sort-Object RouteMetric | Select-Object -First 1
$local = Get-NetIPAddress -InterfaceIndex $route.InterfaceIndex -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike '169.254.*' } | Select-Object -First 1
if (-not $local) { throw 'No LAN IPv4 address found.' }
if (-not $RemoveMapping) {
    $program = Join-Path $PSScriptRoot 'Toy Story 2 Co-op.exe'
    $ruleName = 'ToyStory2-Coop-TCP-27922'
    if (Get-NetFirewallRule -Name $ruleName -ErrorAction SilentlyContinue) { Remove-NetFirewallRule -Name $ruleName }
    New-NetFirewallRule -Name $ruleName -DisplayName $description -Direction Inbound -Action Allow -Protocol TCP -LocalPort $port -Program $program -Profile Any -ErrorAction Stop | Out-Null
    $verified = Get-NetFirewallRule -Name $ruleName -ErrorAction Stop
    if ($verified.Enabled -ne 'True' -or $verified.Action -ne 'Allow') { throw 'Firewall rule verification failed.' }
    Write-Output "Firewall: allowed TCP $port for co-op only. LAN address: $($local.IPAddress)."
}
try {
    $nat = New-Object -ComObject HNetCfg.NATUPnP
    $maps = $nat.StaticPortMappingCollection
    if ($null -eq $maps) { throw 'Router does not expose UPnP port mapping.' }
    $existing = @($maps) | Where-Object { $_.ExternalPort -eq $port -and $_.Protocol -eq 'TCP' } | Select-Object -First 1
    if ($existing -and ($existing.InternalClient -ne $local.IPAddress -or $existing.Description -ne $description)) { throw 'TCP 27922 is already mapped to another application or PC; it was left unchanged.' }
    if ($RemoveMapping) {
        if ($existing) { $maps.Remove($port,'TCP'); Write-Output 'Removed this PC co-op router mapping.' }
    } else {
        if (-not $existing) { $null = $maps.Add($port,'TCP',$port,$local.IPAddress,$true,$description) }
        Write-Output "Router: TCP $port forwarded to $($local.IPAddress). Internet reachability still requires a connection from another network."
    }
} catch {
    Write-Output "Router: $($_.Exception.Message) Forward TCP $port to $($local.IPAddress) manually if needed. A VPN address can also be used."
}
if ($ReportPath) { Stop-Transcript | Out-Null }
