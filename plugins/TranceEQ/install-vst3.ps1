# Installs the built TranceEQ.vst3 bundle into the system VST3 folder.
# Must be run elevated (it writes to C:\Program Files\Common Files\VST3).
#
# Uses robocopy /MIR (mirror) instead of Copy-Item: Copy-Item -Recurse into an existing
# directory NESTS the source as a subfolder (TranceEQ.vst3\TranceEQ.vst3\...) instead of
# overwriting, which silently leaves a stale plugin in place. /MIR mirrors exactly and
# removes any stale/nested files.
$src = 'C:\APC\y\build\plugins\TranceEQ\TranceEQ_artefacts\Release\VST3\TranceEQ.vst3'
$dst = 'C:\Program Files\Common Files\VST3\TranceEQ.vst3'
if (-not (Test-Path $src)) { Write-Error "Source not found: $src"; exit 1 }

& robocopy $src $dst /MIR /NJH /NJS /NP /R:2 /W:1 | Out-Null
$code = $LASTEXITCODE
# robocopy exit codes: 0-7 = success (bits: 1=copied, 2=extras removed, 4=mismatch); >=8 = failure.
if ($code -ge 8) { Write-Error "robocopy failed (exit $code)"; exit 1 }
Write-Host "TranceEQ VST3 installed to $dst (robocopy rc=$code)" -ForegroundColor Green
exit 0
