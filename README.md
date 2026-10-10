# Foundation (Steam 690830) on Apple Silicon via CrossOver

Working setup as of 2026-10-09 (CrossOver 25.1.1, macOS 26.6, M4 Max). Foundation is an OpenGL 4.3-only
game; macOS tops out at 4.1 and CrossOver's D3DMetal/DXMT/DXVK only translate Direct3D. The fix runs Mesa's
Windows OpenGL implementation inside the bottle, with its `d3d12` Gallium driver sitting on top of
CrossOver's D3DMetal.

## Layout inside the bottle ("Foundation" bottle, game folder)

| File | Purpose |
|---|---|
| `opengl32.dll` | **the shim** (`shim/`): forwards all 378 other exports to `mesagl32.dll`, hooks `wglMakeCurrent`, `wglSwapBuffers`, `wglGetProcAddress` |
| `mesagl32.dll` | Mesa 26.2.4 `opengl32.dll` from mesa-dist-win (release-msvc), renamed |
| `libgallium_wgl.dll` | Mesa Gallium, **unmodified** (stock mesa-dist-win 26.2.4 release-msvc) |
| `dxil.dll` | Mesa DXIL signing helper |
| `shim.cfg` | `async=0` (keep it 0; async indirect-readback is unsafe for this game) |
| `fps.log` | written by the shim: fps / frame times / terrain-draw stats every 2 s |

Bottle config (`cxbottle.conf` `[EnvironmentVariables]`): `GALLIUM_DRIVER=d3d12`,
`MESA_GL_VERSION_OVERRIDE=4.3`, `MESA_GLSL_VERSION_OVERRIDE=430`.
Registry: `HKCU\Software\Wine\AppDefaults\foundation.exe\DllOverrides` has `opengl32=native,builtin`,
`dxil=native,builtin`.
Game settings (`~/Documents/Polymorph Games/Foundation/usersetting.config`): windowed 1600x900,
`ClipCursor=false` (fullscreen/exclusive never shows a window under this path).

## What each fix addresses

1. **No window / transparent window** – with an alpha pixel format Mesa's d3d12 WGL presents via
   DirectComposition, which Wine stubs. The shim hooks `wglChoosePixelFormat`/`wglSetPixelFormat` and
   steers the game to the equivalent no-alpha (RGB8, D24S8) format, which presents normally.
   (Earlier versions byte-patched Mesa instead; `upstream/0001-*.patch` is the proper source fix.)
2. **"setStorage bigger than MaxShaderStorageBufferSize"** – Gallium's default cap is 128 MB; the game
   allocates up to 256 MB and only checks the reported value (Mesa doesn't enforce it). The shim reports
   512 MB from `glGetIntegerv`/`glGetInteger64v`. (`upstream/0002-*.patch` fixes it in Mesa.)
3. **Blank UI / no cursor** – the UI/text shaders use `flat` varyings. With GL's default last-vertex
   provoking convention Mesa-d3d12 injects a geometry-shader variant (no `GetAttributeAtVertex` on
   D3DMetal) and those draws render nothing. The shim calls `glProvokingVertex(GL_FIRST_VERTEX_CONVENTION)`
   after each `wglMakeCurrent`.
4. **8 fps in-game** – terrain is drawn with `glMultiDrawElementsIndirect(GL_PATCHES, ~8000 cmds)` several
   times per frame; ExecuteIndirect + tessellation costs ~13 µs per command on D3DMetal vs ~1 µs for a
   direct draw. The shim reads the indirect buffer back once per GPU rewrite (invalidated on
   barriers/dispatches/buffer updates) and replays the commands as direct draws. Result: ~55 fps at medium
   settings (CPU and GPU serialize on the readback; lower settings → higher fps).

## Rebuilding

    zig cc -target x86_64-windows-gnu -O2 -shared -o opengl32.dll shim.c shim.def
    zig cc -target x86_64-windows-gnu -O2 glcheck.c -o glcheck.exe -lopengl32 -lgdi32 -luser32
    zig cc -target x86_64-windows-gnu -O2 winput.c -o winput.exe -luser32

`tools/glcheck.c` is the GL 4.3 feature/benchmark suite used to find the bugs (`glcheck.exe 4 3 compat full`,
`... hold`, `... terrain`). `tools/winput.c` injects input into the game window from inside the bottle;
`scripts/game.sh` drives it and pulls shim screenshots (`touch shot.req` → `shot.ppm`).

Caveats: mesa-dist-win is used unmodified (checksum pinned to 26.2.4); a game update that changes rendering may need
re-tracing (apitrace 14 works under the bottle; `patches/glretrace-patched.exe` has a pixel-format fix).
Known leftovers: timer queries return 0, no shader binary cache (shaders recompile each launch).

## Packaged bottle (`bottle/`, `dist/`)

`bottle/make-bottle.sh` rebuilds the whole thing from scratch on any Apple-silicon Mac with CrossOver and
`7zz` installed: creates the bottle with the env vars, downloads mesa-dist-win 26.2.4 (checksum-verified),
verifies its checksum, installs stock Mesa + the shim into the bottle's own
`C:\windows\system32` (the Steam game folder is never touched, so Steam updates / "verify integrity" can't
undo it), sets the per-exe DLL overrides, silently installs Steam, seeds the game's user settings and can
export the result:

    zsh bottle/make-bottle.sh --name Foundation --export dist/Foundation-bottle.cxarchive

`dist/Foundation-bottle.cxarchive` (~140 MB) is that export: Steam installed but never logged in, no game.
Import it in CrossOver (Bottle → Import Bottle Archive…) or `cxbottle --bottle Foundation --restore
dist/Foundation-bottle.cxarchive`, then run Steam inside it, log in, install Foundation, play. The game
settings seed (`bottle/usersetting.seed.config`: windowed 1600x900, cursor not clipped) is written to
`~/Documents/Polymorph Games/Foundation/` by the script; after an import without the script, copy it there
before the first launch (exclusive fullscreen on first run is untested on this path).

The current working bottle on this machine uses the same system32 layout.

## CrossTie / "official" packaging (`crosstie/`, `installer/`)

`installer/foundation-opengl-fix.exe` (built by `installer/build.sh`, source `fixinstall.c`) is a
self-contained Windows installer: `/S` installs Mesa + shim into the bottle's system32 and registers the
DLL overrides; `/U` removes them. `crosstie/Foundation-OpenGL-Mesa.tie` is a CrossTie with two profiles:
the fix as a *component* (env vars, registry, silent installer) and "Foundation (Steam, OpenGL via Mesa)"
depending on it and on CodeWeavers' Steam profile. Double-click the .tie to install through CrossOver's
normal Install dialog (it will ask for the installer file until `<downloadurl>` points at a hosted copy).

To make it *official*, submit the CrossTie on the Foundation page of the CodeWeavers compatibility
database (it is currently rated "known not to work" on Mac) — community CrossTies are reviewed by
CodeWeavers staff. It ships stock Mesa (MIT) plus the shim (this repo); nothing is binary-patched. `upstream/` holds the
Mesa source patches and bug-report drafts for the D3DMetal issues (provoking-vertex GS path,
ExecuteIndirect + tessellation cost) — the path to making the shim unnecessary.
