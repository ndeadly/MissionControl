#!/usr/bin/env bash
# Run from unpacked project directory after 'gh auth login'. Never embeds credentials.
set -Eeuo pipefail
command -v git >/dev/null || { echo 'git fehlt'; exit 1; }
command -v gh >/dev/null || { echo 'GitHub CLI (gh) fehlt'; exit 1; }
gh auth status >/dev/null || { echo 'Bitte zuerst gh auth login ausführen'; exit 1; }
ROOT="$(cd "$(dirname "$0")" && pwd)"
TEMP="$(mktemp -d)"
trap 'rm -rf "$TEMP"' EXIT
REPO='kaivonderburg-create/MissionControl'
BRANCH='dynamicos-nvr-automation'
gh repo clone "$REPO" "$TEMP/repo" -- --branch "$BRANCH" --single-branch
cd "$TEMP/repo"
for item in nvr android appliance tests .github; do
  test -e "$ROOT/$item" && cp -a "$ROOT/$item" ./
done
for item in README.md SECURITY.md RELEASE-GATES.md RELEASE-NEXT.md RELEASE-STATUS-v18.md AUTOMATION.md; do
  test -f "$ROOT/$item" && cp "$ROOT/$item" ./
done
git add nvr android appliance tests .github *.md
git diff --cached --quiet && { echo 'Keine Änderungen'; exit 0; }
git commit -m 'feat(nvr): publish consolidated source for automated CI'
git push origin "$BRANCH"
echo "Quelle hochgeladen: https://github.com/$REPO/tree/$BRANCH"
