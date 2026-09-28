#!/bin/sh
# Fetches the live device's SPA shell + every tab fragment + the icons each
# fragment references, and assembles one static, fully self-contained HTML
# file per tab under assembled/ - the same DOM a browser would end up with
# after $("#contentmain").load(...), but pre-baked so a later step can
# screenshot it via file:// with zero network access required.
#
# Split from render_preview.js because this needs curl (works in this
# session) rather than Node's own network stack (blocked here even for
# public internet, not just the LAN - see FORK_NOTES.md's web_preview note).
set -e
cd "$(dirname "$0")"

BASE_URL="${BASE_URL:-http://192.168.1.100}"
WEB_USER="${WEB_USER:-admin}"
WEB_PASS="${WEB_PASS:-admin}"

rm -rf assembled
mkdir -p assembled/icons

fetch() {
	curl -fsS --max-time 10 -u "$WEB_USER:$WEB_PASS" "$BASE_URL$1"
}

fetch "/" >assembled/_root.html
fetch "/style.css" >assembled/style.css

TABS="dashboard radio igate digi tracker vpn wireless system file about"
for tab in $TABS; do
	echo "Fetching /$tab ..."
	fetch "/$tab" >"assembled/_frag_$tab.html"

	# Pull down every icon this fragment references and rewrite its <img>
	# src to the local copy - each igate/digi/tracker page only references
	# one (its current symbol preview), so this is cheap.
	grep -o '/icon\.png?[^"'"'"']*' "assembled/_frag_$tab.html" | sort -u | while read -r iconpath; do
		iconfile="icons/$(echo "$iconpath" | tr -c 'a-zA-Z0-9' '_').bmp"
		fetch "$iconpath" >"assembled/$iconfile"
		python3 - "assembled/_frag_$tab.html" "$iconpath" "$iconfile" <<'EOF'
import sys
path, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
with open(path) as f:
    content = f.read()
content = content.replace(old, new)
with open(path, "w") as f:
    f.write(content)
EOF
	done

	python3 - "$tab" <<'EOF'
import sys
tab = sys.argv[1]
with open("assembled/_root.html") as f:
    shell = f.read()
with open(f"assembled/_frag_{tab}.html") as f:
    frag = f.read()
marker = '<div class="contentwide" id="contentmain"  style="font-size: 2pt;">\n\n</div>'
assert marker in shell, "contentmain marker not found - root page structure changed?"
page = shell.replace(marker, f'<div class="contentwide" id="contentmain">\n{frag}\n</div>')
with open(f"assembled/{tab}.html", "w") as f:
    f.write(page)
EOF
done

echo "Done. Assembled pages in assembled/*.html"
