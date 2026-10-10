#!/bin/zsh
# Builds fixinstall.exe with the fix payload appended. Needs: zig, the shim DLL, the patched Mesa files.
# usage: build.sh <dir containing opengl32.dll(shim) mesagl32.dll libgallium_wgl.dll dxil.dll shim.cfg> [out.exe]
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd); src=$1; out=${2:-$here/foundation-opengl-fix.exe}
zig cc -target x86_64-windows-gnu -O2 "$here/fixinstall.c" -o "$here/fixinstall-stub.exe" -ladvapi32 -luser32
cp "$here/fixinstall-stub.exe" "$out"
python3 - "$src" "$out" <<'PY'
import sys, os, struct
src, out = sys.argv[1], sys.argv[2]
payload = b""
for name in ["opengl32.dll", "mesagl32.dll", "libgallium_wgl.dll", "dxil.dll", "shim.cfg"]:
    data = open(os.path.join(src, name), "rb").read()
    payload += struct.pack("<I", len(name)) + name.encode() + struct.pack("<II", len(data) & 0xffffffff, len(data) >> 32) + data
payload += struct.pack("<I", 0)
with open(out, "ab") as f:
    f.write(payload); f.write(struct.pack("<Q", len(payload)) + b"FNDFIX01")
print(out, os.path.getsize(out), "bytes")
PY
