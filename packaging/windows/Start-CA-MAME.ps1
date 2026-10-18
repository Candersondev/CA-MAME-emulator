param(
 [string]$MameExecutable = "",
 [string]$Catalog = "",
 [string]$RomDirectory = ""
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $RomDirectory) { $RomDirectory = Join-Path $root 'roms' }
if (-not $MameExecutable) { $MameExecutable = Join-Path $root 'mame.exe' }
if (-not $Catalog) { $Catalog = Join-Path $root 'catalog.xml' }
New-Item -ItemType Directory -Force -Path $RomDirectory | Out-Null
foreach ($category in @('snap','titles','cabinets')) {
 New-Item -ItemType Directory -Force -Path (Join-Path $RomDirectory "artwork/$category") | Out-Null
}
if (-not (Test-Path -LiteralPath $MameExecutable -PathType Leaf)) {
 Write-Host "MAME executable not found: $MameExecutable" -ForegroundColor Yellow
 Write-Host "Install MAME separately and provide its full path using -MameExecutable."
 Read-Host 'Press Enter to exit'
 exit 2
}
if (-not (Test-Path -LiteralPath $Catalog -PathType Leaf)) {
 Write-Host "Creating MAME catalog (may take a moment)..."
 & $MameExecutable -listxml | Out-File -LiteralPath $Catalog -Encoding utf8
 if ($LASTEXITCODE -ne 0) { throw "MAME -listxml failed: $LASTEXITCODE" }
}
$browser = Join-Path $root 'ca-mame-library.exe'
if (-not (Test-Path -LiteralPath $browser -PathType Leaf)) { throw 'Missing ca-mame-library.exe' }
& $browser $Catalog $RomDirectory $MameExecutable
exit $LASTEXITCODE
