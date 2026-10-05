param([string]$InnoCompiler='', [string]$BinaryPath='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$version='0.2.0'
if(!$BinaryPath) {
    $BinaryPath=Join-Path $root 'build\QuietPin.exe'
    if(!(Test-Path -LiteralPath $BinaryPath)){$BinaryPath=Join-Path $root 'build\Release\QuietPin.exe'}
}
$binary=Get-Item -LiteralPath $BinaryPath
if($binary.VersionInfo.FileVersion -ne $version){throw "Build the matching v$version binary before packaging."}
if(!$InnoCompiler){
    $InnoCompiler=Join-Path $root 'build\tools\InnoSetup\ISCC.exe'
    if(!(Test-Path -LiteralPath $InnoCompiler)){
        $command=Get-Command ISCC.exe -ErrorAction SilentlyContinue
        if($command){$InnoCompiler=$command.Source}
    }
}
if(!(Test-Path -LiteralPath $InnoCompiler)){throw 'Install Inno Setup 6.7+ and pass -InnoCompiler <ISCC.exe>.'}
$release=Join-Path $root "dist\QuietPin-v$version-win-x64"
# Rebuild only this exact package directory so old development documents cannot leak into the ZIP.
if(Test-Path -LiteralPath $release){
    $resolved=[IO.Path]::GetFullPath($release)
    $expected=[IO.Path]::GetFullPath((Join-Path $root "dist\QuietPin-v$version-win-x64"))
    if($resolved -ne $expected -or !(($resolved+[IO.Path]::DirectorySeparatorChar).StartsWith([IO.Path]::GetFullPath($root)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase))){throw 'Unsafe package directory'}
    if((Get-Item -LiteralPath $release).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Package directory must not be a link'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $release | Out-Null
Copy-Item -LiteralPath $binary.FullName -Destination (Join-Path $release 'QuietPin.exe') -Force
foreach($name in @('open-settings.cmd','exit.cmd')){
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $release -Force
}
foreach($name in @('README.md','README.zh.md','CHANGELOG.md')){
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $release -Force
}
$hash=Get-FileHash -LiteralPath (Join-Path $release 'QuietPin.exe') -Algorithm SHA256
[IO.File]::WriteAllText((Join-Path $release 'SHA256SUMS.txt'),($hash.Hash.ToLower()+'  QuietPin.exe'+[Environment]::NewLine),[Text.Encoding]::ASCII)
$zip=Join-Path $root "dist\QuietPin-v$version-win-x64.zip"
Compress-Archive -LiteralPath $release -DestinationPath $zip -Force
& $InnoCompiler "/DAppVersion=$version" "/DSourceRoot=$root" "/DBinaryPath=$($binary.FullName)" (Join-Path $root 'installer\QuietPin.iss')
if($LASTEXITCODE -ne 0){throw 'Installer compilation failed'}
$setup=Join-Path $root "dist\QuietPinSetup-v$version-win-x64.exe"
$lines=@($zip,$setup) | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLower()+'  '+[IO.Path]::GetFileName($_) }
$lines+= $hash.Hash.ToLower()+'  QuietPin.exe'
[IO.File]::WriteAllLines((Join-Path $root "dist\QuietPin-v$version-SHA256SUMS.txt"),$lines,[Text.Encoding]::ASCII)
Get-Item -LiteralPath $zip,$setup | Select-Object FullName,Length
