<#
.SYNOPSIS
  Download the version-pinned pluginval binary into _tools/pluginval/.
  The binary is gitignored; run this once per machine (build-and-install.ps1
  skips validation with a warning when it is missing).
#>
[CmdletBinding()]
param(
    [string]$Version = "v1.0.4",
    [string]$InstallPath = "_tools/pluginval"
)

$ErrorActionPreference = "Stop"
$exe = Join-Path $InstallPath "pluginval.exe"

if (Test-Path $exe) {
    Write-Host "pluginval already present at $exe" -ForegroundColor Green
    exit 0
}

$url = "https://github.com/Tracktion/pluginval/releases/download/$Version/pluginval_Windows.zip"
$tempZip = Join-Path $env:TEMP "pluginval_$Version.zip"

Write-Host "Fetching pluginval $Version ..." -ForegroundColor Cyan
Invoke-WebRequest -Uri $url -OutFile $tempZip
New-Item -ItemType Directory -Force -Path $InstallPath | Out-Null
Expand-Archive -Path $tempZip -DestinationPath $InstallPath -Force
Remove-Item $tempZip -ErrorAction SilentlyContinue

if (-not (Test-Path $exe)) { throw "pluginval.exe not found after extraction" }
Write-Host "pluginval $Version installed at $exe" -ForegroundColor Green
