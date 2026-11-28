# Restore ONLY the original 4 MiB application partition from dual verified
# flash snapshots. No erase-all, bootloader, partition table, NVS or SD writes.
param(
    [Parameter(Mandatory=$true)][string]$RecoveryFolder,
    [Parameter(Mandatory=$true)][string]$ExpectedMac,
    [string]$Port = "COM5",
    [switch]$Install
)
$ErrorActionPreference = "Stop"
try {
    . (Join-Path $PSScriptRoot "tab5_tools.ps1")
    $tools = Get-Tab5Tools
    $python = $tools.Python
    $esp = $tools.Esp
    if ($ExpectedMac -notmatch '^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$') {
        throw "Exact device MAC required."
    }
    $root = (Resolve-Path -LiteralPath $RecoveryFolder).Path
    $backup = Join-Path $root "flash-backup.bin"
    $verify = Join-Path $root "flash-verification.bin"
    $sidecar = "$backup.sha256.txt"
    if (!(Test-Path -LiteralPath $sidecar -PathType Leaf)) {
        $sidecar = Join-Path $root "backup.sha256.txt"
    }
    $expected = (Get-Content -LiteralPath $sidecar -Raw).Trim()
    if ($expected -notmatch '^[0-9a-fA-F]{64}$') { throw "Invalid backup digest." }
    foreach ($file in @($backup,$verify)) {
        if ((Get-Item -LiteralPath $file).Length -ne 16777216 -or
            (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $expected) {
            throw "Backup verification failed: $file"
        }
    }
    $old = [IO.File]::ReadAllBytes($backup)
    # ESP-IDF partition records: magic AA50 LE, type, subtype, offset, size.
    # Require one factory app with the exact known offset and length.
    $matches = 0
    for ($i = 0x8000; $i -lt 0x8C00; $i += 32) {
        if ($old[$i] -ne 0xAA -or $old[$i+1] -ne 0x50) { break }
        if ($old[$i+2] -eq 0 -and $old[$i+3] -eq 0) {
            if ([BitConverter]::ToUInt32($old,$i+4) -ne 0x10000 -or
                [BitConverter]::ToUInt32($old,$i+8) -ne 0x400000) {
                throw "Unexpected factory application partition."
            }
            $matches++
        }
    }
    if ($matches -ne 1 -or $old[0x10000] -ne 0xE9) {
        throw "Backup lacks the expected factory application image."
    }
    $info = & $python $esp --chip esp32p4 --port $Port flash-id
    if ($LASTEXITCODE -ne 0 -or
        ($info -join " ") -notmatch [regex]::Escape($ExpectedMac) -or
        ($info -join " ") -notmatch 'Detected flash size:\s*16\s*MB') {
        throw "Device identity or capacity mismatch."
    }
    # Keep recovery output separate from the immutable original backups.
    $out = Join-Path $root ("restore-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $out | Out-Null
    $live = Join-Path $out "live-partitions.bin"
    & $python $esp --chip esp32p4 --port $Port read-flash 0x8000 0xC00 $live
    if ($LASTEXITCODE -ne 0) { throw "Live partition read failed." }
    $table = [IO.File]::ReadAllBytes($live)
    if ($table.Length -ne 3072) { throw "Unexpected live table length." }
    for ($i = 0; $i -lt $table.Length; $i++) {
        if ($table[$i] -ne $old[0x8000+$i]) { throw "Partition layout mismatch." }
    }
    $app = Join-Path $out "original-app-partition.bin"
    $bytes = New-Object byte[] 4194304
    [Array]::Copy($old,0x10000,$bytes,0,$bytes.Length)
    [IO.File]::WriteAllBytes($app,$bytes)
    $hash = (Get-FileHash -LiteralPath $app -Algorithm SHA256).Hash
    Write-Host "RESTORE PREFLIGHT PASSED: $hash"
    if (!$Install) {
        Write-Host "No flash write. Repeat with -Install after reviewing the recovery folder."
        return
    }
    if ((Read-Host "Type RESTORE APP to replace only the application partition") -cne "RESTORE APP") {
        throw "Restore declined."
    }
    if ((Get-FileHash -LiteralPath $app -Algorithm SHA256).Hash -ne $hash) {
        throw "Extracted application changed."
    }
    & $python $esp --chip esp32p4 --port $Port --baud 460800 --after no-reset write-flash 0x10000 $app
    if ($LASTEXITCODE -ne 0) { throw "Application restore write failed." }
    $readback = Join-Path $out "restored-app-readback.bin"
    & $python $esp --chip esp32p4 --port $Port --baud 460800 read-flash 0x10000 0x400000 $readback
    if ($LASTEXITCODE -ne 0 -or (Get-Item -LiteralPath $readback).Length -ne 4194304 -or
        (Get-FileHash -LiteralPath $readback -Algorithm SHA256).Hash -ne $hash) {
        throw "Restored application readback differs. Preserve recovery evidence."
    }
    Write-Host "APPLICATION RESTORE VERIFIED; confirm normal learner boot." -ForegroundColor Green
} catch {
    Write-Host "RESTORE STOPPED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
