# Third-party notices

## Mesa 3D (mesa-dist-win 26.2.4)
The installer and bottle archive redistribute unmodified binaries from
https://github.com/pal1000/mesa-dist-win (release `26.2.4`, `mesa3d-26.2.4-release-msvc.7z`):
`opengl32.dll` (shipped renamed as `mesagl32.dll`), `libgallium_wgl.dll`, `dxil.dll`.
Mesa is licensed under the MIT license; see https://docs.mesa3d.org/license.html. Copyright (c) the
Mesa contributors. The license text in `LICENSE` applies to Mesa's code as well, with Mesa's own
copyright holders substituted. `dxil.dll` is built from Microsoft's DirectXShaderCompiler
(University of Illinois/NCSA license).

## CrossOver / D3DMetal
Nothing from CrossOver or Apple's Game Porting Toolkit is redistributed; the bottle archive contains
only Mesa, this project's shim, and a fresh Windows Steam installation (Valve's redistributable
installer, never logged in).

## apitrace
`tools/` mention apitrace (https://github.com/apitrace/apitrace, MIT) as a debugging aid; it is not
redistributed here.
