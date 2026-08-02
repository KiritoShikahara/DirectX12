# =============================================================================
#  性能ベンチマークの自動実行スクリプト
#
#  ゲームを無敵モード(--godmode)で一定時間動かし、CPU/GPUの計測値をCSVへ記録して
#  統計を表示する。無敵にしないとプレイヤーが数秒で死んで敵の数が頭打ちになり、
#  本来の高負荷時の性能が計測できないため。
#
#  使い方(リポジトリのルートで実行):
#    powershell -ExecutionPolicy Bypass -File Tools\Perf\run_benchmark.ps1
#    powershell -ExecutionPolicy Bypass -File Tools\Perf\run_benchmark.ps1 -Seconds 90 -SkipBuild
#
#  計測はDevelop構成で行う(Releaseと同じ最適化のままImGui/計測が使えるため)。
#  Debug構成の数値は最適化無効の影響で実性能と数十倍乖離するので使わないこと。
# =============================================================================
param(
    [int]$Seconds = 60,
    [string]$Configuration = "Develop",
    [switch]$SkipBuild,
    [string]$Label = "",
    # 乱数シード。敵の湧き・パーク選択が実行ごとに変わると負荷が変動し、
    # 最適化の前後を比較できなくなるため既定で固定する
    [int]$Seed = 12345
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$appDir = Join-Path $repoRoot "App"
$exePath = Join-Path $repoRoot "x64\$Configuration\App.exe"
$csvPath = Join-Path $appDir "perf_telemetry.csv"
$resultDir = Join-Path $PSScriptRoot "results"

if (-not (Test-Path $resultDir)) { New-Item -ItemType Directory -Path $resultDir | Out-Null }

# --- ビルド ---------------------------------------------------------------
if (-not $SkipBuild) {
    $msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuild)) { throw "MSBuild not found: $msbuild" }

    # 計測用のCSV出力はECSE_PERF_TELEMETRY定義時のみ有効。
    # 定義の変更だけではMSBuildが再コンパイルを判断しないため、明示的にRebuildする
    Write-Host "[build] $Configuration (with telemetry)..." -ForegroundColor Cyan
    $env:CL = "/DECSE_PERF_TELEMETRY"
    & $msbuild (Join-Path $repoRoot "DirectX12.slnx") /t:Rebuild /p:Configuration=$Configuration /p:Platform=x64 /m /nologo /v:minimal |
        Out-File -FilePath (Join-Path $resultDir "build.log") -Encoding utf8
    $buildExit = $LASTEXITCODE
    Remove-Item Env:CL -ErrorAction SilentlyContinue
    if ($buildExit -ne 0) {
        Select-String -Path (Join-Path $resultDir "build.log") -Pattern ": error" | Select-Object -First 10
        throw "Build failed."
    }
}

if (-not (Test-Path $exePath)) { throw "Executable not found: $exePath" }

# --- 実行 -----------------------------------------------------------------
Remove-Item -Path $csvPath -ErrorAction SilentlyContinue

Write-Host "[run] $Seconds seconds (godmode, seed=$Seed)..." -ForegroundColor Cyan
$proc = Start-Process -FilePath $exePath `
    -WorkingDirectory $appDir `
    -ArgumentList "--godmode", "--autoexit=$Seconds", "--seed=$Seed" `
    -PassThru

# 自動終了しなかった場合に備えて余裕を持って待ち、それでも残っていたら停止する
$deadline = (Get-Date).AddSeconds($Seconds + 20)
while (-not $proc.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
    Write-Host "[run] forced stop (autoexit did not fire)" -ForegroundColor Yellow
}
Start-Sleep -Seconds 2

if (-not (Test-Path $csvPath)) {
    throw "No telemetry produced. Was the build made with ECSE_PERF_TELEMETRY defined?"
}

# --- 結果の保存 -----------------------------------------------------------
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$name = if ($Label -ne "") { "$stamp`_$Label.csv" } else { "$stamp.csv" }
$saved = Join-Path $resultDir $name
Copy-Item $csvPath $saved

Write-Host "[saved] $saved" -ForegroundColor Green

# --- 集計 -----------------------------------------------------------------
& (Join-Path $PSScriptRoot "analyze.ps1") -CsvPath $saved
