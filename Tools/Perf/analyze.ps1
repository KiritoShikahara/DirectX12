# =============================================================================
#  ベンチマーク結果(CSV)を集計して表示する。
#
#  ゲームが実際に動いている行だけを対象にする(タイトル表示中やゲームオーバー後は
#  負荷がかからず、平均に混ざると実態が見えなくなるため)。
#  判定にはPhysicsの所要時間を使う。0のフレームはゲームが進行していない。
#
#  使い方:
#    powershell -ExecutionPolicy Bypass -File Tools\Perf\analyze.ps1 -CsvPath <path>
#    powershell -ExecutionPolicy Bypass -File Tools\Perf\analyze.ps1 -CsvPath <a> -ComparePath <b>
# =============================================================================
param(
    [Parameter(Mandatory = $true)][string]$CsvPath,
    [string]$ComparePath = ""
)

$ErrorActionPreference = "Stop"

function Get-ActiveRows([string]$path) {
    $rows = Import-Csv $path
    # Physicsが動いている = ゲームが進行しているフレームのみを対象にする
    $active = $rows | Where-Object { [double]$_.Physics -gt 0.0 }
    if ($active.Count -eq 0) { return $rows }
    return $active
}

function Get-Stats($rows, [string]$column) {
    $values = @($rows | ForEach-Object { [double]$_.$column })
    if ($values.Count -eq 0) { return $null }
    $sorted = $values | Sort-Object
    [pscustomobject]@{
        Avg = ($values | Measure-Object -Average).Average
        Max = ($values | Measure-Object -Maximum).Maximum
        P95 = $sorted[[int][math]::Floor($sorted.Count * 0.95)]
    }
}

function Show-Report([string]$path) {
    $rows = Get-ActiveRows $path
    Write-Host ""
    Write-Host "=== $(Split-Path $path -Leaf)  (active frames: $($rows.Count)) ===" -ForegroundColor Cyan

    $fps = Get-Stats $rows "fps"
    $low = Get-Stats $rows "low1pct"
    $cpu = Get-Stats $rows "cpu_ms"
    $gpu = Get-Stats $rows "gpu_ms"

    Write-Host ("  FPS      avg {0,7:N1}   1%Low avg {1,7:N1}" -f $fps.Avg, $low.Avg)
    Write-Host ("  CPU ms   avg {0,7:N2}   p95 {1,7:N2}   max {2,7:N2}" -f $cpu.Avg, $cpu.P95, $cpu.Max)
    Write-Host ("  GPU ms   avg {0,7:N2}   p95 {1,7:N2}   max {2,7:N2}" -f $gpu.Avg, $gpu.P95, $gpu.Max)

    # CPU側の内訳。重い順に並べると、どこを削るべきかが一目で分かる
    Write-Host "  -- CPU breakdown (avg ms, heaviest first) --"
    $cpuColumns = @(
        "Gameplay Update", "Physics", "Render Collect", "Shadow Pass",
        "Scene Pass (FBX)", "Sprite Pass", "Effect Update", "Effect Draw", "Debug/ImGui"
    )
    $breakdown = foreach ($c in $cpuColumns) {
        $s = Get-Stats $rows $c
        if ($s -ne $null) { [pscustomobject]@{ Name = $c; Avg = $s.Avg; Max = $s.Max } }
    }
    $total = ($breakdown | Measure-Object -Property Avg -Sum).Sum
    foreach ($b in ($breakdown | Sort-Object Avg -Descending)) {
        Write-Host ("    {0,-18} {1,7:N3}  (max {2,6:N2})" -f $b.Name, $b.Avg, $b.Max)
    }
    Write-Host ("    {0,-18} {1,7:N3}" -f "[measured total]", $total)

    # 計測済みの合計とCPU実測の差。大きい場合は未計測の処理が残っている
    $unaccounted = $cpu.Avg - $total
    if ($unaccounted -gt 0.5) {
        Write-Host ("    {0,-18} {1,7:N3}  <- 未計測(V-Sync待ち含む)" -f "[unaccounted]", $unaccounted) -ForegroundColor Yellow
    }

    Write-Host "  -- GPU breakdown (avg ms) --"
    foreach ($c in @("gpu_shadow", "gpu_scene", "gpu_effect", "gpu_sprite")) {
        $s = Get-Stats $rows $c
        if ($s -ne $null) { Write-Host ("    {0,-18} {1,7:N3}" -f $c, $s.Avg) }
    }

    Write-Host "  -- Effekseer --"
    foreach ($c in @("effect_calls", "effect_verts", "effect_instances")) {
        $s = Get-Stats $rows $c
        if ($s -ne $null) { Write-Host ("    {0,-18} avg {1,9:N0}   max {2,9:N0}" -f $c, $s.Avg, $s.Max) }
    }
}

Show-Report $CsvPath

if ($ComparePath -ne "") {
    Show-Report $ComparePath

    Write-Host ""
    Write-Host "=== 比較 (基準 -> 比較対象) ===" -ForegroundColor Cyan
    $a = Get-ActiveRows $CsvPath
    $b = Get-ActiveRows $ComparePath
    foreach ($c in @("cpu_ms", "gpu_ms", "Effect Update", "Effect Draw", "Render Collect", "Gameplay Update")) {
        $sa = Get-Stats $a $c
        $sb = Get-Stats $b $c
        if ($sa -ne $null -and $sb -ne $null) {
            $delta = $sb.Avg - $sa.Avg
            $pct = if ($sa.Avg -ne 0) { ($delta / $sa.Avg) * 100 } else { 0 }
            $color = if ($delta -lt 0) { "Green" } else { "Red" }
            $sign = if ($pct -ge 0) { "+" } else { "" }
            Write-Host ("  {0,-18} {1,7:N3} -> {2,7:N3}  ({3}{4:N1}%)" -f $c, $sa.Avg, $sb.Avg, $sign, $pct) -ForegroundColor $color
        }
    }
}
