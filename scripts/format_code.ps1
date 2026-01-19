# powershell -ExecutionPolicy Bypass -File scripts/format_code.ps1
$clangFormat = "C:\C\Cpp\LLVM20_64\bin\clang-format.exe"
$root = Get-Location
$files = Get-ChildItem -Path $root -Include *.cpp, *.h -Recurse | Where-Object { $_.FullName -notmatch "third_party" -and $_.FullName -notmatch "\.git" }

foreach ($file in $files) {
    Write-Host "Formatting $($file.FullName)"
    & $clangFormat -i $file.FullName
}
