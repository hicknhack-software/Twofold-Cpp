# SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
# SPDX-License-Identifier: Apache-2.0

# powershell -ExecutionPolicy Bypass -File scripts/format_code.ps1
[CmdletBinding()]
param(
    [switch]$HeadersOnly,
    [string]$ClangFormat = "clang-format"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$utf8 = New-Object System.Text.UTF8Encoding($false)
$sourceExtensions = @(".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".qbs", ".js", ".ts", ".py", ".ps1", ".sh", ".twofold")
$cppExtensions = @(".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx")
# These files were copied from Qbs-OnlineSource and retain its MIT license.
$mitPaths = @("third_party/catch2.qbs", "third_party/nlohmann_json.qbs", "third_party/catch2/catch_user_config.hpp")

# Git includes new source files while excluding ignored builds and downloaded dependencies.
$paths = @(& git -C $root -c core.quotepath=false ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw "Could not list source files in $root" }
$files = @($paths | Sort-Object -Unique | ForEach-Object {
    $path = Join-Path $root $_
    if ((Test-Path -LiteralPath $path -PathType Leaf) -and
        $sourceExtensions -contains [System.IO.Path]::GetExtension($path)) {
        Get-Item -LiteralPath $path
    }
})

foreach ($file in $files) {
    $content = [System.IO.File]::ReadAllText($file.FullName)
    $newline = "`n"
    if ($content.Contains("`r`n")) { $newline = "`r`n" }
    $prefix = "//"
    if ($file.Extension -in @(".py", ".ps1", ".sh")) { $prefix = "#" }
    $relativePath = $file.FullName.Substring($root.Length + 1).Replace('\', '/')
    $license = "Apache-2.0"
    if ($relativePath.StartsWith("qbs/imports/online/") -or $mitPaths -contains $relativePath) { $license = "MIT" }
    $header = "$prefix SPDX-FileCopyrightText: 2026 HicknHack Software GmbH${newline}$prefix SPDX-License-Identifier: $license${newline}${newline}"

    # Keep interpreter directives and Python encoding declarations ahead of the header.
    $preamble = ""
    $match = [regex]::Match($content, '\A#![^\r\n]*(?:\r?\n|\z)')
    if ($match.Success) {
        $preamble = $match.Value.TrimEnd([char[]]"`r`n") + $newline
        $content = $content.Substring($match.Length)
    }
    if ($file.Extension -eq ".py") {
        $match = [regex]::Match($content, '\A[ \t]*#[^\r\n]*coding[:=][ \t]*[-\w.]+[^\r\n]*(?:\r?\n|\z)')
        if ($match.Success) {
            $preamble += $match.Value.TrimEnd([char[]]"`r`n") + $newline
            $content = $content.Substring($match.Length)
        }
    }

    if ($content.StartsWith($header)) { continue }

    # Remove only licensing headers; preserve documentation and ordinary leading comments.
    $content = [regex]::Replace($content, '\A\s*/\*\s*(?:SourceMap|Twofold)-(?:Cpp|Qt)\b.*?\*/[ \t]*(?:\r?\n)?', '', 'Singleline')
    $content = [regex]::Replace($content, '\A(?:(?://|#) SPDX-(?:FileCopyrightText|License-Identifier):[^\r\n]*(?:\r?\n|\z))+', '')
    $newContent = $preamble + $header + $content.TrimStart([char[]]"`r`n")
    Write-Host "Updating header in $($file.FullName)"
    [System.IO.File]::WriteAllText($file.FullName, $newContent, $utf8)
}

if ($HeadersOnly) { return }

# Preserve the previous Windows installation path as a fallback.
if ($ClangFormat -eq "clang-format" -and -not (Get-Command $ClangFormat -ErrorAction SilentlyContinue)) {
    $legacyClangFormat = "C:\C\Cpp\LLVM20_64\bin\clang-format.exe"
    if (Test-Path -LiteralPath $legacyClangFormat) { $ClangFormat = $legacyClangFormat }
}
foreach ($file in $files) {
    if ($cppExtensions -notcontains $file.Extension -or
        $file.FullName -match '[/\\]third_party[/\\]') { continue }
    Write-Host "Formatting $($file.FullName)"
    & $ClangFormat -i $file.FullName
    if ($LASTEXITCODE -ne 0) { throw "clang-format failed for $($file.FullName)" }
}
