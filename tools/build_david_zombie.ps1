param([switch]$Install)
$ErrorActionPreference = 'Stop'
$modRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sdkInit = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
Push-Location (Join-Path $modRoot 'source')
try {
    New-Item -ItemType Directory -Path build -Force | Out-Null
    $compiler = 'cl /nologo /EHsc /std:c++17 /O2 /MT /DWIN32 /D_CRT_SECURE_NO_WARNINGS /I hlsdk\dlls /I hlsdk\engine /I hlsdk\common /I hlsdk\public /I hlsdk\pm_shared '
    & cmd.exe /d /c ('call "'+$sdkInit+'" >nul && '+$compiler+'/Fo:build\test_david.obj /Fe:build\test_david.exe test_david.cpp')
    if ($LASTEXITCODE) { throw 'David logic test compilation failed' }
    & .\build\test_david.exe
    if ($LASTEXITCODE) { throw 'David logic checks failed' }
    & cmd.exe /d /c ('call "'+$sdkInit+'" >nul && '+$compiler+'/LD /Fo:build\ /Fe:build\cremation.dll ricochet_proxy.cpp bounce_shotgun.cpp /link /DEF:cremation.def /INCLUDE:_cremation_export_reserve')
    if ($LASTEXITCODE) { throw 'Proxy compilation failed' }
    python ..\tools\finalize_proxy_exports.py
    if ($LASTEXITCODE) { throw 'Legacy export verification failed' }
    if ($Install) {
        $backup = Join-Path $modRoot 'backups\before-hotd2-david'
        New-Item -ItemType Directory -Path $backup -Force | Out-Null
        $old = Join-Path $backup 'cremation.dll'
        if (-not (Test-Path -LiteralPath $old)) { Copy-Item -LiteralPath (Join-Path $modRoot 'dlls\cremation.dll') -Destination $old }
        Copy-Item -LiteralPath .\build\cremation.dll -Destination (Join-Path $modRoot 'dlls\cremation.dll')
        & .\build\test_native.exe (Join-Path $modRoot 'dlls\cremation.dll')
        if ($LASTEXITCODE) { throw 'Installed native proxy validation failed' }
    }
} finally { Pop-Location }
