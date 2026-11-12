param(
    [Parameter(Mandatory = $true)][string]$QtBin,
    [switch]$LocalTest,
    [string]$Iscc = ''
)

$ErrorActionPreference = 'Stop'
if (-not $Iscc) {
    $Iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
              "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
              "${env:LOCALAPPDATA}\Programs\Inno Setup 6\ISCC.exe") |
            Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $Iscc) { throw 'Inno Setup 6 not found. Install it: winget install JRSoftware.InnoSetup' }
}
$root    = Resolve-Path (Join-Path $PSScriptRoot '..')
$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
$stage   = Join-Path $root 'out\stage'
$vendors = Join-Path $root 'vendors'
$sign    = $env:VETHIDA_SIGN_CMD

Write-Host "Vethida $version"

if ($LocalTest) {
    Write-Warning 'LOCAL TEST BUILD: placeholders are not checked. Do not publish this installer.'
} else {
    & node (Join-Path $root 'scripts\release-ready.mjs')
    if ($LASTEXITCODE -ne 0) { throw 'Fill in the values above first.' }
}

& node (Join-Path $root 'scripts\make-eula.mjs')
if ($LASTEXITCODE -ne 0) { throw 'Could not write scripts\eula.txt.' }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found. Install Visual Studio with the C++ workload.' }
& $msbuild (Join-Path $root 'Vethida\vethida.sln') /m /p:Configuration=Release /p:Platform=x64 /t:vethida
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null
Copy-Item (Join-Path $root 'Vethida\build\x64_Release\Vethida.exe') $stage
$ErrorActionPreference = 'Continue'
& (Join-Path $QtBin 'windeployqt.exe') --release --no-translations --no-system-d3d-compiler --no-opengl-sw (Join-Path $stage 'Vethida.exe')
$deployExit = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
if ($deployExit -ne 0) { throw 'windeployqt failed.' }

$tsFiles = Get-ChildItem (Join-Path $root 'Vethida\translations') -Filter 'vethida_*.ts' -ErrorAction SilentlyContinue
if ($tsFiles) {
    $qmDir = New-Item -ItemType Directory -Path (Join-Path $stage 'translations') -Force
    foreach ($ts in $tsFiles) {
        $lang = $ts.BaseName.Substring('vethida_'.Length)
        & (Join-Path $QtBin 'lrelease.exe') $ts.FullName -qm (Join-Path $qmDir "vethida_$lang.qm")
        if ($LASTEXITCODE -ne 0) { throw "lrelease failed for $($ts.Name)." }
        $qtbase = Join-Path $QtBin "..\translations\qtbase_$lang.qm"
        if (Test-Path $qtbase) { Copy-Item $qtbase $qmDir }
    }
}

$licenses = New-Item -ItemType Directory -Path (Join-Path $stage 'licenses')
Copy-Item (Join-Path $root 'legal\*.txt') $licenses

$winsparkle = Join-Path $vendors 'winsparkle\WinSparkle.dll'
if (-not (Test-Path $winsparkle)) { throw "Missing $winsparkle. Without it the release has no updater." }
Copy-Item $winsparkle $stage

if (Test-Path (Join-Path $vendors 'sentry')) { Copy-Item (Join-Path $vendors 'sentry\*') $stage }

if ($sign) {
    $toSign = @('Vethida.exe', 'WinSparkle.dll', 'sentry.dll', 'crashpad_handler.exe') |
        ForEach-Object { Join-Path $stage $_ } | Where-Object { Test-Path $_ }
    foreach ($file in $toSign) {
        Invoke-Expression "$sign `"$file`""
        if ($LASTEXITCODE -ne 0) { throw "Signing $(Split-Path $file -Leaf) failed." }
    }
} else {
    Write-Warning 'VETHIDA_SIGN_CMD is not set: building an unsigned installer for testing.'
}

$isccArgs = @((Join-Path $PSScriptRoot 'Vethida.iss'))
if ($sign) { $isccArgs = @("/Ssigntool=$sign `$f", '/DSign') + $isccArgs }
& $Iscc @isccArgs
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup failed.' }

$installer = Join-Path $root "out\installer\VethidaSetup-$version.exe"
Write-Host ''
Write-Host "Built $installer"
Write-Host 'Next: upload it, then run'
Write-Host "  node vethida.com\scripts\appcast.mjs add --installer `"$installer`" --url <public download url>"
