<#
.SYNOPSIS
  Configure (APC_BUILD_TESTS=ON), build and run the MidBass Catch2 test suite.
  Returns the test exe's exit code so it can gate a commit.
#>
[CmdletBinding()]
param([Parameter(ValueFromRemainingArguments = $true)] $CatchArgs)

$ErrorActionPreference = "Stop"
$Root  = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$Build = Join-Path $Root "build"

Write-Host "== Configure (APC_BUILD_TESTS=ON) ==" -ForegroundColor Cyan
cmake -S "$Root" -B "$Build" -G "Visual Studio 17 2022" -A x64 -DAPC_BUILD_TESTS=ON | Out-Host
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "== Build MidBass_Tests ==" -ForegroundColor Cyan
cmake --build "$Build" --config Release --target MidBass_Tests | Out-Host
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$exe = Join-Path $Build "plugins\MidBass\Tests\MidBass_Tests_artefacts\Release\MidBass_Tests.exe"
if (-not (Test-Path $exe)) { throw "Test executable not found: $exe" }

Write-Host "== Run $exe ==" -ForegroundColor Cyan
& $exe @CatchArgs
$rc = $LASTEXITCODE
Write-Host "== Test exit code: $rc ==" -ForegroundColor ($(if ($rc -eq 0) { "Green" } else { "Red" }))
exit $rc
