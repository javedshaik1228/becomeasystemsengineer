#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
RECORDS_DIR="$ROOT_DIR/learning-records"
SLUG="${1:-}"
TITLE="${2:-}"
SUMMARY="${3:-}"

usage() {
  cat <<'USAGE'
Usage: scripts/record-learning.sh SLUG "Title" ["1-3 sentence summary"]

Create a record only after evidence establishes non-trivial understanding,
corrects a misconception, confirms prior depth, or changes the mission.
Do not use learning records as daily activity logs.
USAGE
}

if [[ "$SLUG" == '-h' || "$SLUG" == '--help' ]]; then
  usage
  exit 0
fi
if [[ ! "$SLUG" =~ ^[a-z0-9]+(-[a-z0-9]+)*$ || -z "$TITLE" ]]; then
  usage >&2
  exit 2
fi
if [[ -z "$SUMMARY" ]]; then
  if [[ ! -t 0 ]]; then
    printf 'error: provide the evidence-backed summary as the third argument.\n' >&2
    exit 2
  fi
  printf 'What was demonstrated or corrected, and why does it change future study?\n> '
  read -r SUMMARY
fi
if [[ -z "${SUMMARY//[[:space:]]/}" ]]; then
  printf 'error: the summary cannot be empty.\n' >&2
  exit 2
fi

highest=0
if [[ -d "$RECORDS_DIR" ]]; then
  while IFS= read -r record; do
    name="$(basename -- "$record")"
    if [[ "$name" =~ ^([0-9]{4})- ]]; then
      number=$((10#${BASH_REMATCH[1]}))
      if ((number > highest)); then highest=$number; fi
    fi
  done < <(find "$RECORDS_DIR" -maxdepth 1 -type f -name '[0-9][0-9][0-9][0-9]-*.md' -print | sort)
fi

next=$((highest + 1))
printf -v sequence '%04d' "$next"
mkdir -p "$RECORDS_DIR"
target="$RECORDS_DIR/$sequence-$SLUG.md"
if [[ -e "$target" ]]; then
  printf 'error: refusing to overwrite %s\n' "$target" >&2
  exit 1
fi

printf '# %s\n\n%s\n' "$TITLE" "$SUMMARY" >"$target"
printf 'Created %s\n' "$target"
