param([string]$Version='0.2.0')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$suffix=[Guid]::NewGuid().ToString('N')
$work=Join-Path $root ("build\安装 便携测试 $suffix")
$install=Join-Path $work 'installed'
$group="QuietPin Smoke $suffix"
$uninstallKey='HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{83C207BC-33D3-4AC0-A31C-74F595C253F4}_is1'
if(Test-Path -LiteralPath $uninstallKey){throw 'An existing QuietPin installation was found. Use a clean test user to avoid replacing its registration.'}
New-Item -ItemType Directory -Path $work | Out-Null
$setup=Join-Path $root "dist\QuietPinSetup-v$Version-win-x64.exe"
$zip=Join-Path $root "dist\QuietPin-v$Version-win-x64.zip"
$uninstall=Join-Path $install 'unins000.exe'
function Install-Package {
    $process=Start-Process -FilePath $setup -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/DIR="'+$install+'"'),('/GROUP="'+$group+'"'),('/LOG="'+(Join-Path $work 'setup.log')+'"')) -WindowStyle Hidden -PassThru -Wait
    if($process.ExitCode -ne 0){throw "Installer failed: $($process.ExitCode)"}
}
function Invoke-Regression([string]$exe){
    foreach($name in @('quietpin_integration.exe','quietpin_pin_integration.exe')){
        & (Join-Path $root "build\$name") $exe
        if($LASTEXITCODE -ne 0){throw "Regression failed: $name"}
    }
}
try {
    Install-Package
    $installed=Join-Path $install 'QuietPin.exe'
    if((Get-FileHash $installed).Hash -ne (Get-FileHash (Join-Path $root 'build\QuietPin.exe')).Hash){throw 'Installed EXE differs'}
    if(!(Test-Path -LiteralPath $uninstallKey)){throw 'Missing per-user uninstall registration'}
    $shortcuts=Join-Path ([Environment]::GetFolderPath('Programs')) $group
    foreach($name in @('QuietPin.lnk','QuietPin Settings.lnk','Exit QuietPin.lnk','Uninstall QuietPin.lnk')){
        if(!(Test-Path -LiteralPath (Join-Path $shortcuts $name))){throw "Missing shortcut: $name"}
    }
    Invoke-Regression $installed
    # Reinstall over the same path exercises the upgrade code and file replacement.
    Install-Package
    Expand-Archive -LiteralPath $zip -DestinationPath (Join-Path $work 'portable')
    $portable=Join-Path $work "portable\QuietPin-v$Version-win-x64\QuietPin.exe"
    if((Get-FileHash $portable).Hash -ne (Get-FileHash $installed).Hash){throw 'Portable and installed EXE differ'}
    Invoke-Regression $portable
    $process=Start-Process -FilePath $uninstall -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -WindowStyle Hidden -Wait -PassThru
    if($process.ExitCode -ne 0){throw "Uninstall failed: $($process.ExitCode)"}
    Start-Sleep -Milliseconds 800
    if((Test-Path -LiteralPath $installed) -or (Test-Path -LiteralPath $uninstallKey) -or (Test-Path -LiteralPath $shortcuts)){throw 'Uninstall left product files, shortcuts or registration'}
    'PASS: per-user installation, upgrade, shortcut entries, installed regression, Chinese/space portable path regression, identical EXE, uninstall cleanup.' | Tee-Object -FilePath (Join-Path $root 'build\package-verification.txt')
} finally {
    if(Test-Path -LiteralPath $uninstall){
        Start-Process -FilePath $uninstall -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -WindowStyle Hidden -Wait | Out-Null
    }
    # Keep only this isolated workspace tree for diagnostic logs; no recursive deletion.
}
