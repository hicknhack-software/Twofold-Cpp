#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

update_header() {
    local file="$1" prefix='//' license='Apache-2.0' final_newline=0 tmp
    case "$file" in
        *.py|*.ps1|*.sh) prefix='#' ;;
    esac
    # These files were copied from Qbs-OnlineSource and retain its MIT license.
    case "$file" in
        qbs/imports/online/*|third_party/catch2.qbs|third_party/nlohmann_json.qbs|third_party/catch2/catch_user_config.hpp)
            license='MIT' ;;
    esac
    [[ "$(tail -c 1 -- "$file"; printf x)" != $'\nx' ]] || final_newline=1
    tmp="$(mktemp)"
    if ! LC_ALL=C awk -v prefix="$prefix" -v license="$license" \
        -v final_newline="$final_newline" -v extension="${file##*.}" '
        function first_line(value, end) {
            end = index(value, "\n")
            return end ? substr(value, 1, end) : value
        }
        function preserve_line( line) {
            line = first_line(text)
            text = substr(text, length(line) + 1)
            sub(/[\r\n]+$/, "", line)
            preamble = preamble line newline
        }
        { text = text $0 "\n" }
        END {
            # Reconstruct the input without adding a final newline to source bodies.
            if (!final_newline && length(text)) text = substr(text, 1, length(text) - 1)
            original = text
            sub(/^\357\273\277/, "", text)
            newline = index(text, "\r\n") ? "\r\n" : "\n"
            header = prefix " SPDX-FileCopyrightText: 2026 HicknHack Software GmbH" newline \
                prefix " SPDX-License-Identifier: " license newline newline

            # Keep interpreter directives and Python encoding declarations first.
            if (substr(text, 1, 2) == "#!") preserve_line()
            if (extension == "py" && first_line(text) ~ /^[ \t]*#[^\r\n]*coding[:=][ \t]*[-[:alnum:]_.]+/)
                preserve_line()
            if (index(text, header) == 1) {
                printf "%s", original
                exit
            }

            # Remove only old project licensing headers, preserving other comments.
            if (text ~ /^[[:space:]]*\/[*][[:space:]]*(SourceMap|Twofold)-(Cpp|Qt)([^[:alnum:]_]|$)/) {
                end = index(text, "*/")
                if (end) {
                    text = substr(text, end + 2)
                    sub(/^[ \t]*(\r?\n)?/, "", text)
                }
            }
            while (text ~ /^(\/\/|#) SPDX-(FileCopyrightText|License-Identifier):/)
                text = substr(text, length(first_line(text)) + 1)
            sub(/^[\r\n]+/, "", text)
            printf "%s%s%s", preamble, header, text
        }
    ' <"$file" >"$tmp"; then
        rm -f -- "$tmp"
        return 1
    fi
    if ! cmp -s -- "$file" "$tmp"; then
        printf 'Updating header in %s\n' "$file"
        # Writing in place preserves executable permissions and existing links.
        if ! cat -- "$tmp" >"$file"; then
            rm -f -- "$tmp"
            return 1
        fi
    fi
    rm -f -- "$tmp"
}

main() (
    headers_only=false
    clang_format=clang-format
    while (($#)); do
        case "$1" in
            --headers-only) headers_only=true; shift ;;
            --clang-format)
                if (($# < 2)); then
                    printf '%s\n' '--clang-format requires an executable path' >&2
                    exit 2
                fi
                clang_format="$2"; shift 2 ;;
            --clang-format=*) clang_format="${1#*=}"; shift ;;
            -h|--help)
                printf '%s\n' \
                    'Usage: format_code.sh [--headers-only] [--clang-format PATH]' \
                    'Update SPDX headers and format C++ sources.'
                exit 0 ;;
            *) printf 'Unknown argument: %s\n' "$1" >&2; exit 2 ;;
        esac
    done

    files_list="$(mktemp)"
    trap 'rm -f -- "$files_list"' EXIT
    # Include new sources, but exclude ignored builds and downloaded dependencies.
    git ls-files -z --cached --others --exclude-standard | sort -zu >"$files_list"
    format_files=()
    while IFS= read -r -d '' file; do
        [[ -f "$file" ]] || continue
        case "$file" in
            *.c|*.cc|*.cpp|*.cxx|*.h|*.hh|*.hpp|*.hxx|*.qbs|*.js|*.ts|*.py|*.ps1|*.sh|*.twofold)
                update_header "$file" ;;
            *) continue ;;
        esac
        case "/$file" in
            */third_party/*) continue ;;
        esac
        case "$file" in
            *.c|*.cc|*.cpp|*.cxx|*.h|*.hh|*.hpp|*.hxx) format_files+=("$file") ;;
        esac
    done <"$files_list"

    "$headers_only" && exit 0
    command -v -- "$clang_format" >/dev/null || {
        printf 'clang-format executable not found: %s\n' "$clang_format" >&2
        exit 1
    }
    for file in "${format_files[@]}"; do
        printf 'Formatting %s\n' "$file"
        "$clang_format" -i "$file"
    done
)

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
