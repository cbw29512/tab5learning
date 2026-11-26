# Prototype repair only. No factory erase, student data deletion or network access.
param(
    [Parameter(Mandatory=$true)][string]$Backup,
    [Parameter(Mandatory=$true)][string]$ExpectedMac,
    [Parameter(Mandatory=$true)][string]$Revision,
    [string]$Port = "COM5",
    [switch]$SyntheticAuthProbe,
    [switch]$Install
)
$ErrorActionPreference = "Stop"
try {
    $repo = Split-Path -Parent $PSScriptRoot
    $pio = "$HOME\.platformio\penv\Scripts\platformio.exe"
    $python = "$HOME\.platformio\penv\Scripts\python.exe"
    $esptool = "$HOME\.platformio\packages\tool-esptoolpy\esptool.py"
    if ($Revision -notmatch '^[a-fA-F0-9]{40}$') { throw "Full revision required." }
    if ($ExpectedMac -notmatch '^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$') { throw "Invalid device identity." }
    if ((Get-Item -LiteralPath $Backup).Length -ne 16777216) { throw "Invalid backup length." }
    if ((Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash -ne
        (Get-Content -LiteralPath "$Backup.sha256.txt" -Raw).Trim()) {
        throw "Recovery backup hash mismatch."
    }
    $head = git -C $repo rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $head -ne $Revision) { throw "Unexpected source revision." }
    $dirty = git -C $repo status --porcelain
    if ($LASTEXITCODE -ne 0 -or $dirty) { throw "Working tree not clean." }
    # Explicit opt-in: this isolated public-key test is not a learner release.
    $environment = "learner_factory_preview"
    if ($SyntheticAuthProbe) {
        $environment = "auth_usb_probe"
        Write-Warning "Synthetic authentication probe only. Learner screen will be unavailable until restored. No student data transfer."
    }
    Write-Host "=== BUILD VERIFIED SOURCE REVISION: $environment ==="
    & $pio run -d "$repo\firmware" -e $environment
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }

    $build = "$repo\firmware\.pio\build\$environment"
    $app = "$build\firmware.bin"
    $table = [IO.File]::ReadAllBytes("$build\partitions.bin")
    $old = [IO.File]::ReadAllBytes($Backup)
    if ($table.Length -lt 160 -or $table.Length -gt 4096) { throw "Invalid table." }
    for ($i = 0; $i -lt $table.Length; $i++) {
        if ($table[$i] -ne $old[0x8000 + $i]) { throw "Backup partition mismatch." }
    }
    $len = (Get-Item -LiteralPath $app).Length
    if ($len -le 0 -or $len -gt 4194304) { throw "Invalid app size." }

    $image = & $python $esptool --chip esp32p4 image-info $app
    if ($LASTEXITCODE -ne 0) { throw "Image validation failed." }
    $imageText = $image -join "`n"
    if ($imageText -notmatch 'Checksum:.*\(valid\)' -or
        $imageText -notmatch 'Validation hash:.*\(valid\)' -or
        $imageText -notmatch 'Flash size:\s*16MB') {
        throw "Image checksum, hash or flash size check failed."
    }
    $hash = (Get-FileHash -LiteralPath $app -Algorithm SHA256).Hash
    Write-Host "Application SHA256: $hash"
    $info = & $python $esptool --chip esp32p4 --port $Port flash-id
    if ($LASTEXITCODE -ne 0 -or ($info -join "`n") -notmatch [regex]::Escape($ExpectedMac)) {
        throw "Device identity check failed."
    }
    # Confirm the currently installed partition table, not only the backup.
    $temp = [IO.Path]::GetTempFileName()
    try {
        & $python $esptool --chip esp32p4 --port $Port read-flash 0x8000 $table.Length $temp
        if ($LASTEXITCODE -ne 0) { throw "Partition readback failed." }
        $live = [IO.File]::ReadAllBytes($temp)
        if ($live.Length -ne $table.Length) { throw "Partition readback length mismatch." }
        for ($i = 0; $i -lt $table.Length; $i++) {
            if ($live[$i] -ne $table[$i]) { throw "Live partition mismatch." }
        }
    } finally {
        Remove-Item -LiteralPath $temp -ErrorAction SilentlyContinue
    }
    if (-not $Install) {
        Write-Host "PREFLIGHT PASSED; no flash write requested."
        return
    }
    if ((Get-FileHash -LiteralPath $app -Algorithm SHA256).Hash -ne $hash) {
        throw "Image changed after validation."
    }
    Write-Host "=== APPLICATION-ONLY REPAIR ==="
    & $python $esptool --chip esp32p4 --port $Port --baud 460800 write-flash 0x10000 $app
    if ($LASTEXITCODE -ne 0) { throw "Flash write failed." }
    if ($SyntheticAuthProbe) {
        Write-Host "PROBE FLASH VERIFIED; synthetic handshake and learner restoration still pending."
    } else {
        Write-Host "FLASH VERIFIED; physical display, touch and reboot acceptance pending."
    }
} catch {
    Write-Error "REPAIR STOPPED: $($_.Exception.Message)"
    exit 1
}
