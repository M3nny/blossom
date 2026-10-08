#!/usr/bin/env sh
set -e

cd "$(dirname "$0")/.."

FILE="data/fb_200M_uint64"
URL="https://dataverse.harvard.edu/api/access/datafile/3811100"

[ -f "$FILE" ] && { echo "already downloaded: $FILE"; exit 0; }

mkdir -p data
echo "downloading fb_200M_uint64.zst"
curl -L --progress-bar -o "$FILE.zst" "$URL"
zstd -d --rm "$FILE.zst" -o "$FILE"
echo "ready: $FILE"
