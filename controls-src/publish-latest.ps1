$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { throw 'GitHub CLI is missing. Install gh and sign in with gh auth login first.' }
$candidates = @(Get-ChildItem -LiteralPath (Join-Path $root 'release') -File | ForEach-Object {
    if ($_.Name -match '^ToyStory2-remaster-(\d+\.\d+\.\d+\.\d+)\.zip$') { [pscustomobject]@{Version=[version]$Matches[1];Path=$_.FullName} }
} | Sort-Object Version -Descending)
if (-not $candidates.Count) { throw 'No prepared release ZIP exists.' }
$version = $candidates[0].Version.ToString()
$notes = Join-Path $PSScriptRoot 'ReleaseNotes.txt'
if ((Get-Content -LiteralPath $notes -TotalCount 1) -ne "Version: $version") { throw 'Release notes do not match the prepared ZIP. Nothing was published.' }
Write-Host "Publishing Toy Story 2 $version. The installed game will not be changed."
& (Join-Path $PSScriptRoot 'publish-release.ps1') -Version $version -NotesFile $notes
if (-not $?) { throw 'Publishing failed. Read the error above.' }
Write-Host 'Done. Open the game launcher to download the published update.'
