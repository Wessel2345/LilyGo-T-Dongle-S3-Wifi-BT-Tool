#!/usr/bin/env bash
set -euo pipefail
# Fetches third-party display driver from the original LilyGO project.
# No bundled upstream driver sources are re-licensed by this repository.
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UPSTREAM="${1:-$PROJECT/.upstream/T-Dongle-S3}"
if [[ ! -d "$UPSTREAM" ]]; then
  command -v git >/dev/null || { echo 'Git is required.' >&2; exit 1; }
  mkdir -p "$(dirname "$UPSTREAM")"
  git clone --depth 1 https://github.com/Xinyuan-LILYGO/T-Dongle-S3.git "$UPSTREAM"
fi
for name in esp_lcd_st7735.c esp_lcd_st7735.h; do
  mapfile -d '' matches < <(find "$UPSTREAM" -type f -name "$name" -not -path '*/.pio/*' -print0)
  if (( ${#matches[@]} == 0 )); then
    echo "Missing $name in $UPSTREAM. Check upstream repository layout." >&2
    exit 1
  fi
  cp "${matches[0]}" "$PROJECT/src/$name"
  echo "Installed $name from ${matches[0]}"
done
printf 'Display driver ready. Build with: pio run -e T-Dongle-S3\n'
