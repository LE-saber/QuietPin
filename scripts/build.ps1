param([ValidateSet('Auto','MSVC','MinGW')][string]$Compiler='Auto', [switch]$SkipTests)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$buildDirectory=Join-Path $root 'build'
if($Compiler -eq 'Auto') { $Compiler=if(Get-Command cl -ErrorAction SilentlyContinue){'MSVC'}else{'MinGW'} }
if($Compiler -eq 'MinGW') {
    & cmake -S $root -B $buildDirectory -G Ninja -DCMAKE_BUILD_TYPE=Release
} else {
    & cmake -S $root -B $buildDirectory -G 'Visual Studio 17 2022' -A x64
}
if($LASTEXITCODE -ne 0){throw 'CMake configuration failed. Use a fresh build folder when switching compilers.'}
& cmake --build $buildDirectory --config Release
if($LASTEXITCODE -ne 0){throw 'Build failed'}
if(!$SkipTests){
    & ctest --test-dir $buildDirectory -C Release --output-on-failure
    if($LASTEXITCODE -ne 0){throw 'Tests failed'}
}
