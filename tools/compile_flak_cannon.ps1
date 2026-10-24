$ErrorActionPreference = 'Stop'

$modRoot = Split-Path -Parent $PSScriptRoot
$gameRoot = Split-Path -Parent $modRoot
$commonRoot = Split-Path -Parent $gameRoot
$blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
$compiler = Join-Path $commonRoot 'Half-Life SDK\Model Tools\studiomdl.exe'
$qc = 'assets-source\flak_cannon_model\flak_cannon.qc'
$sourceModel = Join-Path $modRoot 'assets-source\flak_cannon_model\v_bounce_shotgun.mdl'

if (-not (Test-Path -LiteralPath $blender)) {
    throw "Blender 5.2 was not found: $blender"
}
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "The Half-Life SDK model compiler was not found: $compiler"
}

& $blender --background --python (Join-Path $PSScriptRoot 'prepare_flak_cannon_model.py')
if ($LASTEXITCODE -ne 0) { throw "Blender preparation failed with exit code $LASTEXITCODE" }

python (Join-Path $PSScriptRoot 'convert_flak_texture.py')
if ($LASTEXITCODE -ne 0) { throw "Texture conversion failed with exit code $LASTEXITCODE" }
Remove-Item -LiteralPath (Join-Path $modRoot 'assets-source\flak_cannon_model\flak_cannon_diffuse.png') -Force

Push-Location $modRoot
try {
    & $compiler $qc
    if ($LASTEXITCODE -ne 0) { throw "studiomdl failed with exit code $LASTEXITCODE" }
} finally {
    Pop-Location
}

$compiledModel = Join-Path $modRoot 'models\cremation\v_bounce_shotgun.mdl'
if (-not (Test-Path -LiteralPath $compiledModel)) {
    throw "studiomdl did not produce the expected model: $compiledModel"
}
Copy-Item -LiteralPath $compiledModel -Destination $sourceModel -Force
python (Join-Path $PSScriptRoot 'install_bounce_assets.py')
if ($LASTEXITCODE -ne 0) { throw "Flak Cannon asset installation failed with exit code $LASTEXITCODE" }

Write-Output "Compiled and installed Flak Cannon viewmodel: $compiledModel"
