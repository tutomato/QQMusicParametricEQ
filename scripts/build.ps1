[CmdletBinding()]
param([string]$ZigCompiler='zig',[string]$InnoCompiler,[string]$Version='0.1.0-preview.1')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$dist=Join-Path $repo 'dist'
New-Item -ItemType Directory -Force -Path $dist | Out-Null
& $ZigCompiler cc -target x86-windows-gnu -O2 (Join-Path $repo 'tests\test_dsp.c') -o (Join-Path $dist 'test_dsp.exe')
if($LASTEXITCODE -ne 0){throw 'DSP test build failed'}
& (Join-Path $dist 'test_dsp.exe')
if($LASTEXITCODE -ne 0){throw 'DSP tests failed'}
& $ZigCompiler cc -target x86-windows-gnu -shared -O2 -DUNICODE -D_UNICODE (Join-Path $repo 'src\plugin.c') -o (Join-Path $dist 'dsp_qqparametric.dll') -luser32 -lgdi32 -lshell32 -lcomctl32
if($LASTEXITCODE -ne 0){throw 'Plugin build failed'}
if($InnoCompiler){
 & $InnoCompiler /Q "/DAppVersion=$Version" "/DProjectDir=$repo" (Join-Path $repo 'installer\setup.iss')
 if($LASTEXITCODE -ne 0){throw 'Installer build failed'}
}
Compress-Archive -LiteralPath (Join-Path $dist 'dsp_qqparametric.dll'),(Join-Path $repo 'README.md'),(Join-Path $repo 'LICENSE') -DestinationPath (Join-Path $dist "QQMusicParametricEQ-$Version-windows-x86.zip") -Force
$prefix="QQMusicParametricEQ-$Version"
$files=Get-ChildItem -LiteralPath $dist -File | Where-Object {$_.Name -like "$prefix*" -or $_.Name -eq 'dsp_qqparametric.dll'}
$lines=foreach($file in $files){(Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()+'  '+$file.Name}
[IO.File]::WriteAllLines((Join-Path $dist 'SHA256SUMS.txt'),$lines)
Write-Host "Built: $dist"
