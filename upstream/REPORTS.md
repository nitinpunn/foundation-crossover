# Upstream reports (drafts)

Everything below was found while getting Foundation (Steam 690830, OpenGL 4.3) to run through
mesa-dist-win 26.2.4's `d3d12` driver on CrossOver 25.1.1 / D3DMetal, macOS 26.6, Apple M4 Max.
`tools/glcheck.c` in this repo reproduces each item standalone (build with
`zig cc -target x86_64-windows-gnu -O2 glcheck.c -o glcheck.exe -lopengl32 -lgdi32 -luser32`, run inside
the bottle next to Mesa's `opengl32.dll` with `GALLIUM_DRIVER=d3d12`).

## 1. Mesa MR: d3d12/wgl — no swapchain when DirectComposition is unavailable
Patch: `0001-d3d12-wgl-fall-back-to-HWND-swapchain-without-DComp.patch`. Compile-tested and verified on a
mingw cross-build of 26.2.4 (see BUILD.md): with the patch an alpha-8 pixel format presents normally.

Repro: `glcheck.exe 4 3 core hold 20 1 8` (requests cAlphaBits=8) under Wine → window stays
transparent/unpresented, no GL error, ~3500 "fps" because Present never blocks. With cAlphaBits=0
(`... hold 20 1 0`) it presents normally at the display rate.

## 2. Mesa MR: d3d12 — GL_MAX_SHADER_STORAGE_BLOCK_SIZE is the 128 MB gallium default
Patch: `0002-d3d12-raise-max_shader_buffer_size.patch` (compile-tested, reports 1 GB). Foundation allocates 256 MB SSBOs and refuses
when the reported cap is smaller (`setStorage() with size 268435456, bigger than
MaxShaderStorageBufferSize (134217728)`); Mesa does not enforce the cap, so simply reporting the real
D3D12 limit fixes it.

## 3. D3DMetal / CrossOver: draws with `flat` varyings produce nothing (last-vertex convention)
With GL's default `GL_LAST_VERTEX_CONVENTION`, Mesa-d3d12 emits a geometry-shader variant to reorder
vertices for flat varyings when `GetAttributeAtVertex` (SM 6.1 barycentrics) is unsupported, which is
the case on D3DMetal. Every draw using such a shader renders nothing — no error, no warning.
`glProvokingVertex(GL_FIRST_VERTEX_CONVENTION)` avoids the GS path and the same shader renders.

Repro: `glcheck.exe 4 3 compat full` → section "[program 27 mirror]": "text shader exact (flat float
varying)" FAIL, "exact shader + glProvokingVertex(FIRST)" PASS. Any `flat` type (float/int/vec4) fails.
Reproduced identically on a clean mingw build of 26.2.4, so it is not a mesa-dist-win packaging artifact.
Explicit geometry shaders written by the app do work ("geometry shader (point -> quad)" PASS), so it is
specific to the driver-generated reorder GS (`d3d12_gs_variant.cpp`, `provoking_vertex > 0`). Either
D3DMetal mishandles that GS or Mesa should prefer `load_at_vertex`-less reordering on this adapter.
Suggested Mesa mitigation: driconf/adapter quirk to force first-vertex behaviour when the GS path is
known-broken.

## 4. D3DMetal: ExecuteIndirect with tessellation is ~13 µs per command
`glMultiDrawElementsIndirect(GL_PATCHES)` with 8186 four-vertex patches costs 105 ms; the same commands
as direct `glDrawElements` calls cost 1.2 µs each, as one big draw 0.4 ms, and as indirect triangles
1.1 ms. Foundation's terrain issues ~27 000 such commands per frame → 8 fps.

Repro: `glcheck.exe 4 3 core terrain` prints all variants. Workaround in this repo's shim: read the
indirect buffer back once per GPU rewrite and replay the commands as direct draws (55–65 fps).

## Where to file
- Mesa: https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests (patches 1–2), issue for item 3
  (labels: d3d12).
- CodeWeavers: support ticket / Foundation compatibility page (items 3–4 affect any GL app routed through
  D3DMetal this way); they can forward D3DMetal issues to Apple.
- Apple: Feedback Assistant → Game Porting Toolkit (items 3–4) with the glcheck binary and output.
