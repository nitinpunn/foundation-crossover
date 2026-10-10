#!/bin/zsh
# Puts Mesa's OpenGL (d3d12 driver) next to Foundation's executables and sets per-app DLL overrides.
set -e
S=/private/tmp/claude-501/-Users-nitinpunn-Documents-dev-foundations-game/2808f2bb-ac02-4502-bd44-3977225bd620/scratchpad
CX=/Users/nitinpunn/Applications/CrossOver.app/Contents/SharedSupport/CrossOver
SA="$HOME/Library/Application Support/CrossOver/Bottles/Foundation/drive_c/Program Files (x86)/Steam/steamapps"
installdir=$(grep -E '"installdir"' "$SA/appmanifest_690830.acf" | sed -E 's/.*"installdir"[[:space:]]+"([^"]+)".*/\1/')
GAME="$SA/common/$installdir"
echo "game dir: $GAME"
find "$GAME" -maxdepth 2 -iname "*.exe" | grep -viE "redist|crash|unins|vc_?redist|directx|dotnet|setup" | while read -r exe; do
  dir=$(dirname "$exe"); name=$(basename "$exe")
  cp "$S/dl/mesa/x64/opengl32.dll" "$S/dl/mesa/x64/libgallium_wgl.dll" "$S/dl/mesa/x64/dxil.dll" "$dir/"
  for dll in opengl32 dxil; do
    "$CX/bin/wine" --bottle Foundation -- reg add "HKCU\\Software\\Wine\\AppDefaults\\$name\\DllOverrides" /v $dll /d native,builtin /f >/dev/null 2>&1
  done
  echo "patched: ${exe#$GAME/}"
done
