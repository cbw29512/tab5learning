# Windows PowerShell 5.1. Development hardware only, synthetic SD fixture.
# Reuse the existing installer; never change partitions or erase the card.
param(
    [Parameter(Mandatory=$true)][string]$Revision,
    [Parameter(Mandatory=$true)][string]$ExpectedMac,
    [string]$Port = "COM5"
)
$ErrorActionPreference = "Stop"
try {
    $repo = Split-Path -Parent $PSScriptRoot
    . (Join-Path $PSScriptRoot "tab5_tools.ps1")
    $tools = Get-Tab5Tools
    $python = $tools.Python
    $esptool = $tools.Esp
    $shell = Join-Path $PSHOME "powershell.exe"
    $installer = Join-Path $PSScriptRoot "repair_tab5_preview.ps1"
    if ($Revision -notmatch '^[a-fA-F0-9]{40}$' -or
        $ExpectedMac -notmatch '^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$') {
        throw "Full revision and exact device MAC required."
    }
    foreach ($file in @($python,$esptool,$shell,$installer)) {
        if (!(Test-Path -LiteralPath $file -PathType Leaf)) {
            throw "Missing prerequisite: $file"
        }
    }
    $head = git -C $repo rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $head -ne $Revision) { throw "Wrong source revision." }
    $dirty = git -C $repo status --porcelain
    if ($LASTEXITCODE -ne 0 -or $dirty) { throw "Worktree must be clean." }
    Write-Host "Close all serial monitors and Teacher Studio. Keep USB connected."
    Write-Warning "This temporarily replaces the learner. Restore its backed-up app after the test."
    if ((Read-Host "Confirm original learner is working and SD is independently backed up: type BACKED UP") -ne "BACKED UP") {
        throw "Operator confirmation missing. No flash write."
    }
    # Verify the target before creating backups. Do not assume every COM5
    # device is our 16 MiB development Tab5.
    $info = & $python $esptool --chip esp32p4 --port $Port flash-id
    $code = $LASTEXITCODE
    $info | Write-Host
    $infoText = $info -join "`n"
    if ($code -ne 0 -or $infoText -notmatch [regex]::Escape($ExpectedMac) -or
        $infoText -notmatch 'Detected flash size:\s*16\s*MB') {
        throw "Device or flash capacity verification failed."
    }
    $root = Join-Path $env:LOCALAPPDATA "Tab5Recovery"
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    $recovery = Join-Path $root ((Get-Date -Format "yyyyMMdd-HHmmss") + "-" + [guid]::NewGuid().ToString("N").Substring(0,8))
    New-Item -ItemType Directory -Path $recovery | Out-Null
    $backup = Join-Path $recovery "flash-backup.bin"
    $verify = Join-Path $recovery "flash-verification.bin"
    Write-Host "RECOVERY FOLDER: $recovery"
    # Keep the chip in its loader between independent reads, preventing the
    # running learner from modifying NVS between the two snapshots.
    foreach ($file in @($backup,$verify)) {
        & $python $esptool --chip esp32p4 --port $Port --baud 460800 --after no-reset read-flash 0x0 0x1000000 $file
        if ($LASTEXITCODE -ne 0 -or (Get-Item -LiteralPath $file).Length -ne 16777216) {
            throw "Independent flash read failed. Preserve recovery folder."
        }
    }
    $hash = (Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash
    if ($hash -ne (Get-FileHash -LiteralPath $verify -Algorithm SHA256).Hash) {
        throw "Independent backups differ. No flash write."
    }
    Set-Content -LiteralPath "$backup.sha256.txt" -Value $hash -Encoding ASCII
    Write-Host "DUAL BACKUP VERIFIED: $hash"
    # A separate Windows PowerShell process provides a reliable exit code
    # even if the existing installer exits after an error.
    $arguments = @("-NoProfile","-File",$installer,"-Backup",$backup,
        "-ExpectedMac",$ExpectedMac,"-Revision",$Revision,"-Port",$Port,
        "-SdWriteProbe","-CardBackupConfirmed")
    & $shell @arguments
    if ($LASTEXITCODE -ne 0) { throw "Guarded preflight failed." }
    if ((Read-Host "If PREFLIGHT PASSED, type INSTALL to start the temporary diagnostic") -ne "INSTALL") {
        Write-Host "INSTALL CANCELLED; verified recovery files preserved."
        return
    }
    & $shell @arguments -Install
    if ($LASTEXITCODE -ne 0) { throw "Guarded install failed; preserve recovery files." }
    Write-Host "Record first screen, then restart normally and record second screen."
    Write-Host "Restore the pre-probe learner from: $recovery"
    Write-Host "Use tools/restore_tab5_application.ps1 -RecoveryFolder `"$recovery`" -ExpectedMac $ExpectedMac -Port $Port"
} catch {
    Write-Host "SD TEST STOPPED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
