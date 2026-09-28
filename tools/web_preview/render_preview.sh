#!/bin/sh
# Renders every tab of the live device's web UI to tools/web_preview/out/*.png.
# Needs `npm install` run once in this directory first (installs playwright),
# and a board reachable at BASE_URL (default http://192.168.1.100).
set -e
cd "$(dirname "$0")"
BASE_URL="${BASE_URL:-http://192.168.1.100}" WEB_USER="${WEB_USER:-admin}" WEB_PASS="${WEB_PASS:-admin}" \
	node render_preview.js
