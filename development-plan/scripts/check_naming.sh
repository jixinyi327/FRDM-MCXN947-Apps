#!/usr/bin/env bash
#
# Ensure every versioned or pending repository path is portable and English.
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

invalid=0
while IFS= read -r -d '' path; do
    case "$path" in
        *[!A-Za-z0-9._/-]*)
            printf 'INVALID PATH  %s\n' "$path"
            invalid=$((invalid + 1))
            ;;
    esac
done < <(git -C "$REPO_ROOT" ls-files --cached --others --exclude-standard -z)

if (( invalid > 0 )); then
    printf '\nFAIL  %d repository path(s) contain non-ASCII characters, whitespace, or unsupported symbols.\n' "$invalid"
    printf 'Use ASCII letters, digits, dot, underscore, hyphen, and slash only.\n'
    exit 1
fi

printf 'PASS  all versioned or pending repository paths use portable ASCII names.\n'
