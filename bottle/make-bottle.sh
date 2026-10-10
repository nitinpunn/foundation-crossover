#!/bin/zsh
# Builds a CrossOver bottle that runs Foundation (Steam 690830) on Apple Silicon.
#
#   make-bottle.sh [--name NAME] [--export FILE.cxarchive] [--no-steam] [--skip-game-config]
#
# What it does:
#   1. creates a win10_64 bottle with the Mesa environment variables
#   2. downloads mesa-dist-win 26.2.4 (release-msvc, used unmodified) and installs the
#      Mesa OpenGL stack plus the shim into the bottle's own C:\windows\system32 (so the Steam game
#      folder is never modified and Steam updates/verification can't undo the fix)
#   3. registers per-exe DLL overrides for foundation.exe (opengl32, dxil -> native)
#   4. silently installs Windows Steam into the bottle
#   5. seeds Foundation's user settings (windowed mode, cursor not clipped) in ~/Documents
#   6. optionally exports the bottle as a .cxarchive (before any Steam login, so it is shareable)
#
# Afterwards: open the bottle in CrossOver, run Steam, log in, install Foundation, play.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
name=Foundation; export_to=""; do_steam=1; do_cfg=1
while (( $# )); do case $1 in
  --name) name=$2; shift 2;;
  --export) export_to=$2; shift 2;;
  --no-steam) do_steam=0; shift;;
  --skip-game-config) do_cfg=0; shift;;
  *) echo "unknown option $1" >&2; exit 2;;
esac; done

cxapp=""; for c in "$HOME/Applications/CrossOver.app" /Applications/CrossOver.app; do [[ -d $c ]] && { cxapp=$c; break; }; done
[[ -n $cxapp ]] || { echo "CrossOver.app not found" >&2; exit 1; }
CX="$cxapp/Contents/SharedSupport/CrossOver"
bottle_dir="$HOME/Library/Application Support/CrossOver/Bottles/$name"
[[ -d $bottle_dir ]] && { echo "bottle '$name' already exists at $bottle_dir" >&2; exit 1; }
command -v 7zz >/dev/null || { echo "need 7zz (brew install sevenzip)" >&2; exit 1; }
[[ -f $here/shim-opengl32.dll ]] || { echo "missing $here/shim-opengl32.dll (build with: zig cc -target x86_64-windows-gnu -O2 -shared -o opengl32.dll ../shim/shim.c ../shim/shim.def)" >&2; exit 1; }

work=$(mktemp -d "${TMPDIR:-/tmp}/foundation-bottle.XXXXXX"); trap 'rm -rf "$work"' EXIT
mesa_url=https://github.com/pal1000/mesa-dist-win/releases/download/26.2.4/mesa3d-26.2.4-release-msvc.7z
steam_url=https://cdn.akamai.steamstatic.com/client/installer/SteamSetup.exe

echo "== 1. creating bottle '$name'"
"$CX/bin/cxbottle" --bottle "$name" --create --template win10_64 \
  --description "Foundation (OpenGL 4.3 via Mesa d3d12 on D3DMetal)" \
  --param "EnvironmentVariables:GALLIUM_DRIVER=d3d12" \
  --param "EnvironmentVariables:MESA_GL_VERSION_OVERRIDE=4.3" \
  --param "EnvironmentVariables:MESA_GLSL_VERSION_OVERRIDE=430" >/dev/null
grep -q 'GALLIUM_DRIVER' "$bottle_dir/cxbottle.conf" || {
  # older cxbottle builds ignore --param for the env section; append directly
  perl -0pi -e 's/(\[EnvironmentVariables\]\n)/$1"GALLIUM_DRIVER" = "d3d12"\n"MESA_GL_VERSION_OVERRIDE" = "4.3"\n"MESA_GLSL_VERSION_OVERRIDE" = "430"\n/' "$bottle_dir/cxbottle.conf"
}

echo "== 2. Mesa"
mkdir -p "$work/mesa"; curl -sSL -o "$work/mesa/mesa.7z" "$mesa_url"
want=$(awk '/mesa3d-26.2.4-release-msvc.7z/{print $1}' "$here/mesa-sha256.txt")
have=$(shasum -a 256 "$work/mesa/mesa.7z" | awk '{print $1}')
[[ -z $want || $want == $have ]] || { echo "mesa archive checksum mismatch ($have)" >&2; exit 1; }
7zz x -y -o"$work/mesa" "$work/mesa/mesa.7z" x64/opengl32.dll x64/libgallium_wgl.dll x64/dxil.dll >/dev/null
have=$(shasum -a 256 "$work/mesa/x64/libgallium_wgl.dll" | awk '{print $1}'); want=$(awk '/libgallium_wgl.dll/{print $1}' "$here/mesa-sha256.txt"); [[ -z $want || $want == $have ]] || { echo "libgallium_wgl.dll checksum mismatch" >&2; exit 1; }
sys="$bottle_dir/drive_c/windows/system32"
cp "$here/shim-opengl32.dll"                   "$sys/opengl32.dll"      # shim: forwards to mesagl32; fixes pixel format, SSBO cap, provoking vertex, indirect patch draws
cp "$work/mesa/x64/opengl32.dll"               "$sys/mesagl32.dll"      # Mesa's opengl32 (renamed)
cp "$work/mesa/x64/libgallium_wgl.dll"         "$sys/libgallium_wgl.dll"   # stock Mesa, unmodified
cp "$work/mesa/x64/dxil.dll"                   "$sys/dxil.dll"
printf 'async=0\n' > "$sys/shim.cfg"

echo "== 3. DLL overrides for foundation.exe"
for dll in opengl32 dxil; do
  "$CX/bin/wine" --bottle "$name" --wait-children -- reg add "HKCU\\Software\\Wine\\AppDefaults\\foundation.exe\\DllOverrides" /v $dll /d native,builtin /f >/dev/null 2>&1
done

if (( do_steam )); then
  echo "== 4. Steam"
  curl -sSL -o "$bottle_dir/drive_c/users/Public/SteamSetup.exe" "$steam_url"
  "$CX/bin/wine" --bottle "$name" --wait-children -- 'C:\users\Public\SteamSetup.exe' /S >/dev/null 2>&1 || true
  rm -f "$bottle_dir/drive_c/users/Public/SteamSetup.exe"
  "$CX/bin/cxmenu" --bottle "$name" --sync >/dev/null 2>&1 || true
  [[ -f "$bottle_dir/drive_c/Program Files (x86)/Steam/Steam.exe" ]] && echo "   Steam installed" || echo "   warning: Steam install did not complete; install it from CrossOver's 'Install Application' instead"
fi

if (( do_cfg )); then
  echo "== 5. Foundation user settings"
  cfgdir="$HOME/Documents/Polymorph Games/Foundation"; cfg="$cfgdir/usersetting.config"
  if [[ -f $cfg ]]; then
    sed -i '' -e 's|<DisplayMode value="FULLSCREEN"/>|<DisplayMode value="WINDOWED"/>|' -e 's|<ClipCursor value="true"/>|<ClipCursor value="false"/>|' "$cfg"
    echo "   adjusted existing $cfg"
  else
    mkdir -p "$cfgdir"; cp "$here/usersetting.seed.config" "$cfg"; echo "   seeded $cfg"
  fi
fi

if [[ -n $export_to ]]; then
  echo "== 6. exporting $export_to"
  "$CX/bin/cxbottle" --bottle "$name" --tar "$export_to"
  ls -la "$export_to"
fi
echo "done. Next: CrossOver -> bottle '$name' -> run Steam, log in, install Foundation, play."
