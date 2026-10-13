param([string]$ZigExe)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $ZigExe) { $ZigExe = (Get-Command zig -ErrorAction Stop).Source }
$zigCompiler = (Resolve-Path -LiteralPath $ZigExe).Path
$zigDirectory = Split-Path -Parent $zigCompiler
$zigHeaders = Join-Path $zigDirectory 'lib/libc/include/any-windows-any'
$outputDirectory = Join-Path $repoRoot 'build'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $outputDirectory 'zig-cache'
$pluginSource = Join-Path $repoRoot 'src/POP3ChainFix.c'
$pluginOutput = Join-Path $outputDirectory 'POP3ChainFix.asi'
& $zigCompiler cc -target x86-windows-gnu -shared -nostdlib -Os -msse2 -mfpmath=sse -fno-builtin -Wall -Wextra -Werror -isystem $zigHeaders '-Wl,--entry,DllMain@12' -o $pluginOutput $pluginSource -lkernel32
if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed' }
Get-FileHash -LiteralPath $pluginOutput -Algorithm SHA256
