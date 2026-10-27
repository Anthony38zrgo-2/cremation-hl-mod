param([switch]$Install)
$ErrorActionPreference = 'Stop'
$modRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceRoot = Join-Path $modRoot 'source'
$sdkInit = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
$compiler = 'cl /nologo /EHsc /std:c++17 /O2 /MT /DWIN32 /D_CRT_SECURE_NO_WARNINGS /I hlsdk\dlls /I hlsdk\engine /I hlsdk\common /I hlsdk\public /I hlsdk\pm_shared'
Push-Location $sourceRoot
try {
    foreach ($variant in @(@{ Name='production'; Define='' }, @{ Name='validation'; Define='/DCREMATION_ENABLE_VALIDATION' })) {
        $output = Join-Path 'build' $variant.Name
        New-Item -ItemType Directory -Path $output -Force | Out-Null
        $defines = if ($variant.Define) { ' ' + $variant.Define } else { '' }
        $command = 'call "' + $sdkInit + '" >nul && ' + $compiler + $defines + ' /LD /Fo:' + $output + '\ /Fe:' + $output + '\cremation.dll ricochet_proxy.cpp bounce_shotgun.cpp /link /DEF:cremation.def /INCLUDE:_cremation_export_reserve'
        & cmd.exe /d /c $command
        if ($LASTEXITCODE) { throw "Server DLL compilation failed: $($variant.Name)" }
        python ..\tools\finalize_proxy_exports.py ($output + '\cremation.dll')
        if ($LASTEXITCODE) { throw "Export finalization failed: $($variant.Name)" }
    }
    if ($Install) {
        Copy-Item -LiteralPath 'build\production\cremation.dll' -Destination '..\dlls\cremation.dll' -Force
    }
} finally { Pop-Location }
