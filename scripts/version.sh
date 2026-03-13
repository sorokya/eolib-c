#!/usr/bin/env bash
# Usage: ./scripts/version.sh <new-version>
# Updates the version number in all relevant project files.

set -euo pipefail

NEW="${1:?Usage: $0 <new-version>}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

OLD=$(sed -n 's/.*set(EOLIB_VERSION "\([^"]*\)").*/\1/p' CMakeLists.txt)

if [[ -z "$OLD" ]]; then
  echo "error: could not determine current version from CMakeLists.txt" >&2
  exit 1
fi

if [[ "$OLD" == "$NEW" ]]; then
  echo "Already at version $NEW, nothing to do."
  exit 0
fi

echo "Bumping $OLD -> $NEW"

sed_inplace() {
  if [[ "$OSTYPE" == "darwin"* ]]; then
    sed -i '' "$@"
  else
    sed -i "$@"
  fi
}

FILES=(
  CMakeLists.txt
  Doxyfile
  vcpkg.json
  docs/getting_started.dox
  docs/getting_started_lua.dox
)

for f in "${FILES[@]}"; do
  sed_inplace "s/${OLD}/${NEW}/g" "$f"
  echo "  updated $f"
done

echo "Done. Version is now $NEW."
