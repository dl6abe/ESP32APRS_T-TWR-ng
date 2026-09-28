#!/bin/sh
# Exports a clean, history-free snapshot of this repo's tracked files into a
# SEPARATE, standalone git repo meant to be pushed to the public GitHub
# fork. Deliberately does not touch this repo's own git history or remotes -
# the private repo (this one) never gets a remote pointing at the public
# fork, so a normal `git push` here can never leak private history there.
#
# What gets copied: everything `git ls-files` tracks (so .gitignore'd build
# artifacts are already excluded), minus the explicit excludes below
# (Claude Code instruction files and this project's private backlog notes).
#
# Usage:
#   tools/export_public.sh <target-dir> ["commit message"]
#
# First run: target-dir is created and `git init`'d.
# Later runs: target-dir must already be a git repo (its own `origin` set up
# separately, e.g. `git remote add origin git@github.com:<you>/<repo>.git`);
# this script only stages+commits, it never pushes - review the diff and
# push yourself.
set -e
cd "$(dirname "$0")/.."

TARGET="$1"
MSG="${2:-Update from private development repo}"

if [ -z "$TARGET" ]; then
	echo "Usage: $0 <target-dir> [\"commit message\"]" >&2
	exit 1
fi

# Files/dirs excluded from the public export beyond .gitignore, as decided
# 2026-09-28/29: Claude Code instruction files, and this project's private
# backlog/reasoning notes (kept only in the local/personal repo).
EXCLUDES='^CLAUDE\.md$|^\.claude/|^Findings\.md$|^Findings_resolved\.md$|^FORK_NOTES\.md$|^ARCHITECTURE\.md$'

mkdir -p "$TARGET"
if [ ! -d "$TARGET/.git" ]; then
	echo "Initializing new git repo at $TARGET"
	git init "$TARGET"
fi

echo "Syncing tracked files (git ls-files, minus excludes) into $TARGET ..."
# Build a staging dir with exactly the files that should end up in the
# public export, then mirror it into TARGET with --delete so files removed
# here (or newly excluded) disappear from the export too. Done as a plain
# copy loop rather than rsync --include/--exclude globs, which don't
# reliably preserve parent-directory structure for deeply nested paths
# across rsync versions.
STAGING="$(mktemp -d)"
trap 'rm -rf "$STAGING"' EXIT

git ls-files | grep -vE "$EXCLUDES" | while IFS= read -r f; do
	mkdir -p "$STAGING/$(dirname "$f")"
	cp "$f" "$STAGING/$f"
done
rsync -a --delete --exclude '.git' "$STAGING/" "$TARGET/"

cd "$TARGET"
git add -A
if git diff --cached --quiet; then
	echo "Nothing changed - export is already up to date."
	exit 0
fi

git commit -m "$MSG"
echo ""
echo "Committed to $TARGET. Review with:"
echo "  cd $TARGET && git log --stat -1"
echo "Push yourself when ready - this script never pushes."
