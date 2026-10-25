param([switch]$EngineCheck)
$ErrorActionPreference = 'Stop'
$modPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sdkInit = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
Push-Location (Join-Path $modPath 'source')
try {
    python ..\tools\install_purple_zombie.py
    if ($LASTEXITCODE) { throw 'Purple normal zombie installation failed' }
    python ..\tools\install_quake_zombie.py
    if ($LASTEXITCODE) { throw 'Quake asset/map installation failed' }
    python ..\tools\generate_proxy_exports.py
    if ($LASTEXITCODE) { throw 'Export generation failed' }
    $compiler = 'cl /nologo /EHsc /std:c++17 /O2 /MT /DWIN32 /D_CRT_SECURE_NO_WARNINGS /I hlsdk\dlls /I hlsdk\engine /I hlsdk\common /I hlsdk\public /I hlsdk\pm_shared '
    $commands = @(
        '/Fo:build\ /Fe:build\test_quake_zombie.exe test_quake_zombie.cpp bounce_shotgun.cpp',
        '/Fo:build\test_native.obj /Fe:build\test_native.exe test_native.cpp',
        '/LD /Fo:build\ /Fe:build\cremation.dll ricochet_proxy.cpp bounce_shotgun.cpp /link /DEF:cremation.def /INCLUDE:_cremation_export_reserve'
    )
    foreach ($arguments in $commands) {
        & cmd.exe /d /c ('call "' + $sdkInit + '" >nul && ' + $compiler + $arguments)
        if ($LASTEXITCODE) { throw 'Win32 compilation failed' }
    }
    & .\build\test_quake_zombie.exe
    if ($LASTEXITCODE) { throw 'Quake zombie behavior checks failed' }
    python ..\tools\finalize_proxy_exports.py
    if ($LASTEXITCODE) { throw 'Export finalization failed' }
    Copy-Item -LiteralPath build\cremation.dll -Destination ..\dlls\cremation.dll
    & .\build\test_native.exe ..\dlls\cremation.dll
    if ($LASTEXITCODE) { throw 'Native ABI checks failed' }
    python ..\tools\verify_quake_zombie.py
    if ($LASTEXITCODE) { throw 'Quake replacement verification failed' }
    if ($EngineCheck) {
        python ..\tools\check_server_startup.py --seconds 20
        if ($LASTEXITCODE) { throw 'Normal GoldSrc startup failed' }
        python ..\tools\check_quake_zombie_runtime.py
        if ($LASTEXITCODE) { throw 'Real GoldSrc combat checks failed' }
    }
} finally { Pop-Location }
