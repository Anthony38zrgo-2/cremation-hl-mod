$ErrorActionPreference = 'Stop'
$modSource = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\source'))
$sdkInit = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
Push-Location $modSource
try {
    python ..\tools\generate_proxy_exports.py
    if ($LASTEXITCODE) { throw 'Export generation failed' }
    $compilerBase = 'cl /nologo /EHsc /std:c++17 /O2 /MT /DWIN32 /D_CRT_SECURE_NO_WARNINGS /I hlsdk\dlls /I hlsdk\engine /I hlsdk\common /I hlsdk\public /I hlsdk\pm_shared'
    New-Item -ItemType Directory -Path build -Force | Out-Null
    $configTestCommand = 'call "' + $sdkInit + '" >nul && ' + $compilerBase + ' /Fo:build\test_weapon_config.obj /Fe:build\test_weapon_config.exe test_weapon_config.cpp'
    & cmd.exe /d /c $configTestCommand
    if ($LASTEXITCODE) { throw 'Configuration test compilation failed' }
    & .\build\test_weapon_config.exe
    if ($LASTEXITCODE) { throw 'Configuration validation failed' }
    $testCommand = 'call "' + $sdkInit + '" >nul && ' + $compilerBase + ' /Fo:build\ /Fe:build\test_ricochet.exe test_ricochet.cpp bounce_shotgun.cpp'
    & cmd.exe /d /c $testCommand
    if ($LASTEXITCODE) { throw 'Test compilation failed' }
    & .\build\test_ricochet.exe
    if ($LASTEXITCODE) { throw 'Ricochet behavior checks failed' }
    $bounceTestCommand = 'call "' + $sdkInit + '" >nul && ' + $compilerBase + ' /Fo:build\test_bounce.obj /Fe:build\test_bounce.exe test_bounce.cpp'
    & cmd.exe /d /c $bounceTestCommand
    if ($LASTEXITCODE) { throw 'Bounce Shotgun test compilation failed' }
    & .\build\test_bounce.exe
    if ($LASTEXITCODE) { throw 'Bounce Shotgun behavior checks failed' }
    $dllCommand = 'call "' + $sdkInit + '" >nul && ' + $compilerBase + ' /LD /Fo:build\ /Fe:build\cremation.dll ricochet_proxy.cpp bounce_shotgun.cpp /link /DEF:cremation.def /INCLUDE:_cremation_export_reserve'
    & cmd.exe /d /c $dllCommand
    if ($LASTEXITCODE) { throw 'DLL compilation failed' }
    python ..\tools\finalize_proxy_exports.py
    if ($LASTEXITCODE) { throw 'Legacy export table finalization failed' }
    python ..\tools\build_mod.py
    if ($LASTEXITCODE) { throw 'Mod installation failed' }
    $nativeCommand = 'call "' + $sdkInit + '" >nul && ' + $compilerBase + ' /Fo:build\test_native.obj /Fe:build\test_native.exe test_native.cpp'
    & cmd.exe /d /c $nativeCommand
    if ($LASTEXITCODE) { throw 'Native test compilation failed' }
    & .\build\test_native.exe ..\dlls\cremation.dll
    if ($LASTEXITCODE) { throw 'Native DLL loading check failed' }
    $gameRoot = [System.IO.Path]::GetFullPath((Join-Path $modSource '..\..'))
    & C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /nologo /target:winexe /platform:x86 /r:System.Windows.Forms.dll "/out:$gameRoot\Cremation.exe" ..\tools\Launcher.cs
    if ($LASTEXITCODE) { throw 'Launcher compilation failed' }
    python ..\tools\verify_mod.py
    if ($LASTEXITCODE) { throw 'Mod validation failed' }
} finally { Pop-Location }
