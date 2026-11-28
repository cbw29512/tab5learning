# Shared Windows PowerShell 5.1 tooling for guarded physical previews.
# Prefer the exact package versions already verified on the development PC.
function Get-Tab5Tools {
    $python = "$HOME\.platformio\penv\Scripts\python.exe"
    $pio = "$HOME\.platformio\penv\Scripts\platformio.exe"
    $pinned = "$HOME\.platformio\packages\tool-esptoolpy@src-df27165f86ed49e6fad18103dba8e4c3\esptool.py"
    $legacy = "$HOME\.platformio\packages\tool-esptoolpy\esptool.py"
    $esp = if (Test-Path -LiteralPath $pinned -PathType Leaf) { $pinned } else { $legacy }
    foreach ($file in @($python,$pio,$esp)) {
        if (!(Test-Path -LiteralPath $file -PathType Leaf)) {
            throw "Required Tab5 tool unavailable: $file"
        }
    }
    $compiler = "$HOME\.platformio\packages\toolchain-riscv32-esp@src-6036ff87cea02067dcd10ba37afd476a\bin"
    if (Test-Path -LiteralPath $compiler -PathType Container) {
        $env:Path = "$compiler;$env:Path"
    }
    return @{ Python=$python; Pio=$pio; Esp=$esp }
}
