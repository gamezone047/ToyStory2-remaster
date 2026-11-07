param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+\.\d+$')][string]$Version,
    [Parameter(Mandatory=$true)][string]$NotesFile,
    [switch]$VerifyOnly
)
$ErrorActionPreference='Stop'
$repo='gamezone047/ToyStory2-remaster'
$payloadRepo='gamezone047/ToyStory2-remaster-files'
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$zip=Join-Path $root "release\ToyStory2-remaster-$Version.zip"
$checksum=$zip+'.sha256'
$launcher=Join-Path $root 'release\Toy.Story.2.Launcher.exe'
foreach($file in @($zip,$checksum,$launcher)){if(-not(Test-Path -LiteralPath $file)){throw "Missing release file: $file"}}
if((Get-Content -LiteralPath $checksum -Raw).Split(' ')[0].Trim() -ne (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash){throw 'ZIP checksum mismatch'}
# Verify the standalone EXE is exactly the launcher included in the game package.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead($zip)
try {
    $entry=$archive.GetEntry('Toy Story 2 Launcher.exe');if(-not $entry){throw 'Launcher missing from ZIP'}
    $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
    try{$embedded=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
    if($embedded -ne (Get-FileHash -LiteralPath $launcher -Algorithm SHA256).Hash){throw 'Standalone launcher does not match the ZIP'}
}finally{$archive.Dispose()}
$tag='v'+$Version
function New-ReadmePayload($metadata) {
    $sha=[string]$metadata.sha
    if($sha -notmatch '\A[0-9a-fA-F]{40}\z'){throw 'Invalid README revision returned by GitHub'}
    return @{
        message='Point downloads to standalone launcher'
        sha=$sha
        content=[Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'RepositoryREADME.md')))
    }
}
function Releases([string]$repository) {
    $json=gh api "repos/$repository/releases?per_page=100"
    if($LASTEXITCODE -ne 0){throw "Cannot inspect $repository releases"}
    return ($json | ConvertFrom-Json)
}
$mainReleases=@(Releases $repo)
foreach($r in $mainReleases){if($r.tag_name -match '^v(\d+\.\d+\.\d+\.\d+)$' -and [version]$Matches[1] -gt [version]$Version -and -not $r.draft){throw 'A newer release is already published'}}
# Executed only when the user runs the Desktop publishing BAT.
$savedPreference=$ErrorActionPreference;$ErrorActionPreference='Continue'
gh api "repos/$payloadRepo" --silent 2>$null
$ErrorActionPreference=$savedPreference
if($LASTEXITCODE -ne 0) {
    if($VerifyOnly){throw 'Game-file repository does not exist. Verification made no changes.'}
    gh repo create $payloadRepo --public --add-readme --description 'Automatic game downloads for the Toy Story 2 Remaster launcher. Download the launcher from gamezone047/ToyStory2-remaster.'
    if($LASTEXITCODE -ne 0){throw 'Cannot create the game-download repository. Nothing was published.'}
}
function Prepare([string]$repository,[string[]]$files,[string]$title,[string]$notes) {
    $existing=@(Releases $repository | Where-Object tag_name -eq $tag)
    if(-not $existing.Count){gh release create $tag @files --repo $repository --draft --title $title --notes-file $notes}
    elseif($existing[0].draft){
        gh release upload $tag @files --repo $repository --clobber
        if($LASTEXITCODE -ne 0){throw "Draft upload failed in $repository"}
        gh release edit $tag --repo $repository --title $title --notes-file $notes
    }
    else {return}
    if($LASTEXITCODE -ne 0){throw "Upload failed in $repository. Previous releases retained."}
}
function Verified([string]$repository,[string[]]$files) {
    $match=@(Releases $repository | Where-Object tag_name -eq $tag)
    if($match.Count -ne 1){throw "Cannot identify new release in $repository"}
    $release=$match[0]
    if(@($release.assets).Count -ne $files.Count){throw "Unexpected assets in $repository release"}
    foreach($file in $files){
        $asset=@($release.assets | Where-Object name -eq ([IO.Path]::GetFileName($file)))
        $digest='sha256:'+(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        if($asset.Count -ne 1){throw "Asset name mismatch in ${repository}: expected '$([IO.Path]::GetFileName($file))'; found '$($release.assets.name -join ', ')'."}
        if($asset[0].state -ne 'uploaded' -or $asset[0].digest -ne $digest){throw "Asset verification failed in ${repository}: $($asset[0].name), state=$($asset[0].state), expected=$digest, actual=$($asset[0].digest)"}
    }
    return $release
}
if($VerifyOnly){
    $null=Verified $payloadRepo @($zip,$checksum)
    $null=Verified $repo @($launcher)
    $metadata=gh api "repos/$repo/contents/README.md" | ConvertFrom-Json
    if($LASTEXITCODE -ne 0){throw 'Cannot inspect README revision'}
    $check=New-ReadmePayload $metadata | ConvertTo-Json | ConvertFrom-Json
    if($check.sha -cne $metadata.sha -or [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($check.content)) -cne [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'RepositoryREADME.md'))){throw 'README payload serialization failed'}
    Write-Host 'PASS: release assets match local files; README request contains the exact revision and document. No uploads or publication performed.'
    return
}
Prepare $payloadRepo @($zip,$checksum) "$Version - Automatic game files" $NotesFile
Prepare $repo @($launcher) "$Version - Download the launcher" $NotesFile
$payload=Verified $payloadRepo @($zip,$checksum)
$main=Verified $repo @($launcher)
# Publish data first, so the visible launcher never points to a draft download.
gh api --method PATCH "repos/$payloadRepo/releases/$($payload.id)" -F draft=false -f make_latest=true --jq .html_url
if($LASTEXITCODE -ne 0){throw 'Game-file publication failed. Main release not changed.'}
gh api --method PATCH "repos/$repo/releases/$($main.id)" -F draft=false -f make_latest=true --jq .html_url
if($LASTEXITCODE -ne 0){throw 'Launcher publication failed. Previous release retained.'}
$latest=gh api "repos/$repo/releases/latest" | ConvertFrom-Json
if($LASTEXITCODE -ne 0 -or $latest.id -ne $main.id){throw 'New launcher is not latest; old releases retained'}
# Update the repository's download button only after the direct EXE link is live.
$readme=gh api "repos/$repo/contents/README.md" | ConvertFrom-Json
if($LASTEXITCODE -ne 0){throw 'Release is live, but the repository download link could not be inspected.'}
$requestFile=Join-Path $env:TEMP ('Toy2-readme-'+[guid]::NewGuid().ToString('N')+'.json')
try {
    $json=New-ReadmePayload $readme | ConvertTo-Json
    [IO.File]::WriteAllText($requestFile,$json,(New-Object Text.UTF8Encoding($false)))
    gh api --method PUT "repos/$repo/contents/README.md" --input $requestFile --silent
    if($LASTEXITCODE -ne 0){throw 'Release is live, but README download-link update failed. Run the BAT again to retry with the current README revision.'}
} finally {if(Test-Path -LiteralPath $requestFile){Remove-Item -LiteralPath $requestFile -Force}}
foreach($repository in @($repo,$payloadRepo)){
    foreach($old in @(Releases $repository)){
        if($old.tag_name -match '^v(\d+\.\d+\.\d+\.\d+)$' -and [version]$Matches[1] -lt [version]$Version){
            gh release delete $old.tag_name --repo $repository --yes --cleanup-tag
            if($LASTEXITCODE -ne 0){throw "New build is live, but old release cleanup failed in $repository"}
        }
    }
}
Write-Host 'Published: main release contains only the launcher; game downloads are in the separate files repository.'
Write-Host 'Direct download: https://github.com/gamezone047/ToyStory2-remaster/releases/latest/download/Toy.Story.2.Launcher.exe'


