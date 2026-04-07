#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
TEMPLATE="$DIR/glove80_dvorak.kle.json"
SCRIPT="$DIR/render_kle.py"

# Primary layers (BASE, NAV, NUM, SYM) — the default cheat sheet
python3 "$SCRIPT" \
  --template "$TEMPLATE" \
  --output "$DIR/glove80_dvorak.kle.json"

# Secondary layers (TYPING, FUNC, SYS, MBO)
python3 "$SCRIPT" \
  --template "$TEMPLATE" \
  --output "$DIR/glove80_dvorak_secondary.kle.json" \
  --layers TYPING,FUNC,SYS,MBO
