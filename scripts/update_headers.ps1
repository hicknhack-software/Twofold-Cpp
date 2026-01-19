# powershell -ExecutionPolicy Bypass -File scripts/update_headers.ps1
$files = Get-ChildItem -Path . -Include *.h,*.cpp -Recurse
foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    $originalContent = $content
    $content = $content -replace '/\* Twofold-Qt', '/* Twofold-Cpp'
    $content = $content -replace '\(C\) Copyright \d+ HicknHack Software GmbH', '(C) Copyright 2025 HicknHack Software GmbH'
    $content = $content -replace 'https://github.com/hicknhack-software/Twofold-Qt', 'https://github.com/hicknhack-software/Twofold-Cpp'

    if ($content -ne $originalContent) {
        Write-Host "Updating $($file.FullName)"
        $content | Set-Content $file.FullName -NoNewline
    }
}
