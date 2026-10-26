$ErrorActionPreference = 'Stop'
$modPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sdkInit = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
Push-Location (Join-Path $modPath 'source')
try {
    & cmd.exe /d /c ('call "' + $sdkInit + '" >nul && cl /nologo /LD /EHsc /std:c++17 /O2 /MT /DWIN32 /D_CRT_SECURE_NO_WARNINGS /I hlsdk\cl_dll /I hlsdk\engine /I hlsdk\common /I hlsdk\public /I hlsdk\pm_shared /Fo:build\client_proxy.obj client_proxy.cpp /link /DEF:client_proxy.def opengl32.lib winmm.lib /OUT:build\client.dll')
    if ($LASTEXITCODE) { throw 'Client effect compilation failed' }
    $backup = Join-Path $modPath 'backups\before-knife-radar'
    New-Item -ItemType Directory -Force $backup | Out-Null
    if (!(Test-Path -LiteralPath (Join-Path $backup 'client.dll'))) { Copy-Item -LiteralPath ..\cl_dlls\client.dll -Destination (Join-Path $backup 'client.dll') }
    Copy-Item -LiteralPath build\client.dll -Destination ..\cl_dlls\client.dll
} finally { Pop-Location }
