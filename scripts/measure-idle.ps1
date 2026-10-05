param([int]$Seconds=60)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $root 'build\QuietPin.exe'
$profile=Join-Path $root ('build\idle-profile-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $profile | Out-Null
$config=@'
[General]
SchemaVersion=1
Language=en
ShowStatus=0
ShowTray=0
StartWithWindows=0
[Hotkeys]
ToggleModifiers=7
ToggleKey=121
[Exclusions]
Count=0
'@
[IO.File]::WriteAllText((Join-Path $profile 'settings.ini'),$config,[Text.Encoding]::Unicode)
$process=Start-Process -FilePath $exe -ArgumentList @('--config-dir',('"'+$profile+'"')) -WindowStyle Hidden -PassThru
try {
    Start-Sleep -Seconds 2
    $process.Refresh()
    if($process.HasExited){throw 'QuietPin exited before measurement'}
    $initialCpu=$process.TotalProcessorTime.TotalSeconds
    $initialPrivate=$process.PrivateMemorySize64
    $timer=[Diagnostics.Stopwatch]::StartNew()
    Start-Sleep -Seconds $Seconds
    $process.Refresh()
    [pscustomobject]@{
        elapsedSeconds=[Math]::Round($timer.Elapsed.TotalSeconds,3)
        cpuSeconds=[Math]::Round($process.TotalProcessorTime.TotalSeconds-$initialCpu,6)
        oneCoreCpuPercent=[Math]::Round(100*($process.TotalProcessorTime.TotalSeconds-$initialCpu)/$timer.Elapsed.TotalSeconds,5)
        logicalProcessors=[Environment]::ProcessorCount
        initialPrivateBytes=$initialPrivate
        finalPrivateBytes=$process.PrivateMemorySize64
        workingSetBytes=$process.WorkingSet64
        handles=$process.HandleCount
        threads=$process.Threads.Count
        exeBytes=(Get-Item $exe).Length
    } | ConvertTo-Json | Tee-Object -FilePath (Join-Path $root 'build\idle-measurement.json')
} finally {
    & $exe --config-dir $profile --exit
    if(!$process.WaitForExit(3000)){ $process.Kill() }
    # Delete only this script's known file and now-empty profile, never a recursive computed target.
    Remove-Item -LiteralPath (Join-Path $profile 'settings.ini') -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $profile -ErrorAction SilentlyContinue
}
