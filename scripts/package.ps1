$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$release=Join-Path $root 'dist\QuietPin-v0.1.0-win-x64'
New-Item -ItemType Directory -Force -Path $release | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'build\QuietPin.exe') -Destination $release -Force
foreach($name in @('open-settings.cmd','exit.cmd')){
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $release -Force
}
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $release -Force
Copy-Item -LiteralPath (Join-Path $root 'docs\verification.zh-CN.md') -Destination $release -Force
Copy-Item -LiteralPath (Join-Path $root 'docs\technical-design.zh-CN.md') -Destination $release -Force
$hash=Get-FileHash -LiteralPath (Join-Path $release 'QuietPin.exe') -Algorithm SHA256
[IO.File]::WriteAllText((Join-Path $release 'SHA256SUMS.txt'),($hash.Hash.ToLower()+'  QuietPin.exe'+[Environment]::NewLine),[Text.Encoding]::ASCII)
$zip=Join-Path $root 'dist\QuietPin-v0.1.0-win-x64.zip'
Compress-Archive -LiteralPath $release -DestinationPath $zip -Force
Get-Item -LiteralPath $zip | Select-Object FullName,Length
