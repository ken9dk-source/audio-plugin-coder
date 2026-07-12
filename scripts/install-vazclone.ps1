# install-vazclone.ps1 — copy the freshly built VAZClone VST3 into the system VST3 folder.
# Self-elevates (UAC prompt) because C:\Program Files\Common Files\VST3 needs admin.
$src = "C:\APC\y\build\plugins\VAZClone\VAZClone_artefacts\Release\VST3\VAZClone.vst3\Contents\x86_64-win\VAZClone.vst3"
$dst = "C:\Program Files\Common Files\VST3\VAZClone.vst3"

if (-not (Test-Path $src)) { Write-Host "Build not found: $src" -ForegroundColor Red; exit 1 }

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
         ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) {
    Start-Process powershell "-ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

Copy-Item $src $dst -Force
Write-Host "Installed VAZClone.vst3 -> $dst  ($((Get-Item $dst).Length) bytes)" -ForegroundColor Green
Write-Host "Rescan/re-open in FL Studio to pick up the new build." -ForegroundColor Green
