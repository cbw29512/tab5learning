# Local adaptive regression gate

GitHub Actions remains the full firmware/USB/security regression gate. If runs
are queued, developers can run the exact portable curriculum/adaptive tests
locally on a clean checkout. No hardware, SD, student records or network
required by this runner.

## Native C++ tests

From the repository root, on a machine with Python 3 and a native g++ C++17
compiler available on PATH:

```sh
python -m tools.run_adaptive_checks
```

Use `python3` if required by the host. For another compatible native compiler,
pass `--compiler clang++` or the path to the compiler. Do not use the ESP32
cross-compiler: host binaries must execute on the computer. Windows requires
a compatible native toolchain; Linux works with the usual g++ installation.
The runner does not install software. It checks exact generated curriculum
data and compiles/runs 11 native suites in a temporary directory, with
warnings-as-errors, timeouts, and nonzero exit if any suite fails.

## Build-only physical target

On the existing Windows development PC, from the clean repository root:

```powershell
. .\tools\tab5_tools.ps1
$tools = Get-Tab5Tools
& $tools.Pio run -d firmware -e learner_factory_preview
if ($LASTEXITCODE -ne 0) { throw "Firmware build failed" }
```

This builds the normal physical Tab5 target only. It does not upload firmware.
A build pass is not proof of hardware acceptance, USB security or SD durability.
Do not install a failed build or bypass the guarded installer.

## Acceptance order

1. Record exact source revision, native regression result and build output.
2. Test normal preview display stability, touch, resume, achievements and
   optional extra practice using adult-only synthetic inputs.
3. Follow SD_COMPLETION_QUALIFICATION.md on a separate backed-up card.
4. Only after hardware acceptance consider a guarded SD evaluation.
5. Full CI, authenticated USB, teacher review, privacy and recovery gates still
   apply before child beta. Never substitute synthetic history for actual
   student evidence.
