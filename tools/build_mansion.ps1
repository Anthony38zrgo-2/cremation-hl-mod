$ErrorActionPreference = 'Stop'
$modPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sdkInit = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
Push-Location (Join-Path $modPath 'source')
try {
    $compiler = 'cl /nologo /EHsc /std:c++17 /O2 /MT /DWIN32 /D_CRT_SECURE_NO_WARNINGS /I hlsdk\dlls /I hlsdk\engine /I hlsdk\common /I hlsdk\public /I hlsdk\pm_shared '
    foreach ($arguments in @(
        '/Fo:build\test_mansion_waves.obj /Fe:build\test_mansion_waves.exe test_mansion_waves.cpp',
        '/LD /Fo:build\ /Fe:build\cremation.dll ricochet_proxy.cpp bounce_shotgun.cpp /link /DEF:cremation.def /INCLUDE:_cremation_export_reserve'
    )) {
        & cmd.exe /d /c ('call "' + $sdkInit + '" >nul && ' + $compiler + $arguments)
        if ($LASTEXITCODE) { throw 'Mansion Win32 compilation failed' }
    }
    & .\build\test_mansion_waves.exe
    if ($LASTEXITCODE) { throw 'Mansion composition checks failed' }
    python ..\tools\finalize_proxy_exports.py
    if ($LASTEXITCODE) { throw 'Export finalization failed' }
    Copy-Item -LiteralPath build\cremation.dll -Destination ..\dlls\cremation.dll
    & .\build\test_native.exe ..\dlls\cremation.dll
    if ($LASTEXITCODE) { throw 'Native ABI checks failed' }
    & 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe' /nologo /target:winexe /out:..\launcher\Cremation.exe /reference:System.Windows.Forms.dll ..\tools\Launcher.cs
    if ($LASTEXITCODE) { throw 'Launcher compilation failed' }
    Copy-Item -LiteralPath ..\launcher\Cremation.exe -Destination ..\..\Cremation.exe
} finally { Pop-Location }
