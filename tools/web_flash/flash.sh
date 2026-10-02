#!/bin/sh
# Flashes firmware.bin to a running T-TWR board over its web UI's OTA
# endpoint (POST /update, webservice.cpp), instead of a USB/serial upload -
# useful for a board that's only reachable over WiFi (e.g. mounted on a
# tower as a repeater/iGate).
#
# This restarts the board and briefly interrupts RF/iGate service. The
# board may be unattended, deployed hardware (see CLAUDE.md/FORK_NOTES.md) -
# don't run this against a production device without knowing that's
# acceptable right now. The script refuses to proceed past the confirmation
# prompt unless BASE_URL is set explicitly (no guessed/default device IP)
# and either the prompt is accepted or --yes is passed.
#
# Uses curl for all network I/O, not any Node-based HTTP client - a sandboxed
# dev environment this has been run from before allow-lists specific
# binaries for network access (curl works, Node's own fetch/http did not
# reach anything, LAN or public internet) - see tools/web_preview's
# FORK_NOTES.md section for the same caveat in more detail. If curl itself
# can't reach BASE_URL from wherever this runs, there's no workaround here;
# flash from a machine that's actually on the device's LAN instead (e.g. via
# the browser upload form at http://<device-ip>/file, or run this script
# from the user's own machine rather than an agent sandbox).
#
# Usage:
#   BASE_URL=http://192.168.1.50 tools/web_flash/flash.sh
#   BASE_URL=http://192.168.1.50 WEB_USER=admin WEB_PASS=admin tools/web_flash/flash.sh --yes
#   BASE_URL=http://192.168.1.50 tools/web_flash/flash.sh --no-build --fw-path some/other/firmware.bin
#
# Flags:
#   --yes         Skip the confirmation prompt (for real automation/CI use).
#   --no-build    Skip "pio run" and flash whatever's already at --fw-path.
#   --no-wait     Skip polling the device after upload to confirm it came
#                 back up; just report the upload result and exit.
#   --fw-path P   Firmware binary to flash (default: PlatformIO's own output
#                 path for env esp32s3box, resolved from this repo root).
set -e
cd "$(dirname "$0")/../.."
REPO_ROOT="$(pwd)"

WEB_USER="${WEB_USER:-admin}"
WEB_PASS="${WEB_PASS:-admin}"
FW_PATH="$REPO_ROOT/.pio/build/esp32s3box/firmware.bin"
DO_BUILD=1
SKIP_CONFIRM=0
DO_WAIT=1

while [ $# -gt 0 ]; do
	case "$1" in
	--yes) SKIP_CONFIRM=1 ;;
	--no-build) DO_BUILD=0 ;;
	--no-wait) DO_WAIT=0 ;;
	--fw-path)
		shift
		FW_PATH="$1"
		;;
	*)
		echo "Unknown argument: $1" >&2
		exit 1
		;;
	esac
	shift
done

if [ -z "$BASE_URL" ]; then
	echo "BASE_URL is not set - refusing to guess a device address." >&2
	echo "Usage: BASE_URL=http://<device-ip> $0 [--yes] [--no-build] [--no-wait] [--fw-path PATH]" >&2
	exit 1
fi
# Strip a trailing slash so "$BASE_URL/update" below doesn't end up with //.
BASE_URL="${BASE_URL%/}"

if [ "$DO_BUILD" -eq 1 ]; then
	echo "Building firmware (pio run -e esp32s3box)..."
	pio run -e esp32s3box
fi

if [ ! -f "$FW_PATH" ]; then
	echo "Firmware not found at $FW_PATH" >&2
	if [ "$DO_BUILD" -eq 0 ]; then
		echo "(ran with --no-build - build it first, or pass --fw-path)" >&2
	fi
	exit 1
fi
FW_SIZE=$(wc -c <"$FW_PATH" | tr -d ' ')
echo "Firmware: $FW_PATH ($FW_SIZE bytes)"

echo "Checking device is reachable at $BASE_URL ..."
if ! curl -fsS -m 5 -o /dev/null -u "$WEB_USER:$WEB_PASS" "$BASE_URL/"; then
	echo "Could not reach $BASE_URL - aborting before touching flash." >&2
	exit 1
fi
echo "Device reachable."

if [ "$SKIP_CONFIRM" -ne 1 ]; then
	printf 'Flash %s to %s now? This restarts the board and briefly interrupts RF/iGate service. [y/N] ' "$FW_PATH" "$BASE_URL"
	read -r REPLY
	case "$REPLY" in
	[Yy]*) ;;
	*)
		echo "Aborted."
		exit 1
		;;
	esac
fi

echo "Uploading (device will restart on success)..."
# Field name must be "update" - matches the file <input name="update">
# on the device's own File tab upload form (web_misc.cpp), which is what
# the /update handler (webservice.cpp) expects.
RESPONSE=$(curl -fsS -m 120 -u "$WEB_USER:$WEB_PASS" \
	-F "update=@${FW_PATH};type=application/octet-stream" \
	"$BASE_URL/update")

if [ "$RESPONSE" != "OK" ]; then
	echo "Device reported: $RESPONSE" >&2
	echo "Upload did not report OK - check the device before assuming it flashed cleanly." >&2
	exit 1
fi
echo "Device reported OK, rebooting."

if [ "$DO_WAIT" -eq 1 ]; then
	echo "Waiting for the device to come back up..."
	i=0
	while [ "$i" -lt 30 ]; do
		i=$((i + 1))
		sleep 2
		if curl -fsS -m 3 -o /dev/null -u "$WEB_USER:$WEB_PASS" "$BASE_URL/"; then
			echo "Device is back up ($((i * 2))s after upload)."
			exit 0
		fi
	done
	echo "Device did not come back within 60s - check it directly (console/serial if reachable)." >&2
	exit 1
fi
