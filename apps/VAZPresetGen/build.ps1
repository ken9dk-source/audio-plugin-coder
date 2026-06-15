# Build the VAZ AI Preset Generator into a standalone Windows .exe.
#   powershell -ExecutionPolicy Bypass -File .\build.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

Write-Host "[1/3] Installing dependencies..." -ForegroundColor Cyan
py -m pip install --upgrade pip | Out-Host
py -m pip install --upgrade customtkinter pyinstaller | Out-Host

Write-Host "[2/3] Running engine self-test..." -ForegroundColor Cyan
py tests/test_engine.py | Out-Host

Write-Host "[3/3] Building .exe with PyInstaller..." -ForegroundColor Cyan
py -m PyInstaller --noconfirm --clean VAZPresetGen.spec | Out-Host

$exe = Join-Path $PSScriptRoot "dist\VAZ Preset Generator.exe"
if (Test-Path $exe) {
    Write-Host "`nBUILD OK -> $exe" -ForegroundColor Green
    Write-Host "Double-click it to run. No Python or terminal needed on the target machine."
} else {
    Write-Host "`nBUILD FAILED — see PyInstaller output above." -ForegroundColor Red
    exit 1
}
