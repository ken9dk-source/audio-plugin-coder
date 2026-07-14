# Phase-A: dump DFM property bytes around the Sequencer timing/range controls, so their
# Min/Max/Position/Increment (small ints) are readable next to the property names.
param([string]$Path = 'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll')
$bytes = [System.IO.File]::ReadAllBytes($Path)
$anchors = @('sbSwing','sbGateTime','sbStepPage','udFinalStep','msTempo','msTimebase','cbStartStep','cbEndStep')
function Find-All([byte[]]$hay, [string]$needle) {
  $pat = [System.Text.Encoding]::ASCII.GetBytes($needle); $hits = @()
  for ($i = 0; $i -le $hay.Length - $pat.Length; $i++) {
    $ok = $true
    for ($j = 0; $j -lt $pat.Length; $j++) { if ($hay[$i+$j] -ne $pat[$j]) { $ok = $false; break } }
    if ($ok) { $hits += $i }
  }
  return $hits
}
foreach ($a in $anchors) {
  $hits = Find-All $bytes $a
  Write-Host ("===== {0}  ({1} hit) =====" -f $a, $hits.Count)
  $shown = 0
  foreach ($h in $hits) {
    if ($shown -ge 1) { break }
    $start = [Math]::Max(0, $h - 2); $end = [Math]::Min($bytes.Length - 1, $h + 90)
    $sb = New-Object System.Text.StringBuilder
    for ($k = $start; $k -le $end; $k++) {
      $b = $bytes[$k]
      if ($b -ge 0x20 -and $b -le 0x7E) { [void]$sb.Append([char]$b) } else { [void]$sb.Append(('<{0:X2}>' -f $b)) }
    }
    Write-Host ('  @0x{0:X}: {1}' -f $h, $sb.ToString()); $shown++
  }
}
