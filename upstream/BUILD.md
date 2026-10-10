# Cross-building Mesa (d3d12 + WGL only) on macOS to test the patches

Toolchain (Homebrew): `mingw-w64 meson ninja bison flex`, plus a Python with `mako pyyaml packaging`
on PATH ahead of the system one. Takes ~30 s to compile on an M4 Max.

    curl -LO https://archive.mesa3d.org/mesa-26.2.4.tar.xz && tar xf mesa-26.2.4.tar.xz
    cd mesa-26.2.4
    patch -p1 < ../0001-d3d12-wgl-fall-back-to-HWND-swapchain-without-DComp.patch
    patch -p1 < ../0002-d3d12-raise-max_shader_buffer_size.patch
    export PATH=/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:$PATH
    meson setup ../build-win --cross-file ../cross-mingw64.txt --buildtype=release \
      -Dgallium-drivers=d3d12 -Dvulkan-drivers= -Dplatforms=windows -Dllvm=disabled \
      -Dgles1=disabled -Dgles2=disabled -Dshared-glapi=enabled -Dgallium-d3d12-video=disabled \
      -Dmicrosoft-clc=disabled -Dzstd=disabled -Dshader-cache=disabled -Dvalgrind=disabled \
      -Dlibunwind=disabled -Dbuild-tests=false -Dgallium-va=disabled
    ninja -C ../build-win

Outputs: `build-win/src/gallium/targets/libgl-gdi/opengl32.dll`, `build-win/src/gallium/targets/wgl/libgallium_wgl.dll`,
`build-win/subprojects/zlib-1.3.1/libz-1.dll`. At runtime the mingw build also needs `libwinpthread-1.dll`
(from the mingw-w64 package) and `dxil.dll` (take it from mesa-dist-win).

## Results (2026-10-09, CrossOver 25.1.1, macOS 26.6, M4 Max)

Run with `GALLIUM_DRIVER=d3d12 MESA_GL_VERSION_OVERRIDE=4.3 MESA_GLSL_VERSION_OVERRIDE=430` and no shim:

| test (`tools/glcheck.c`) | stock 26.2.4 | patched build |
|---|---|---|
| `hold` with cAlphaBits=8 (#63) | never presented, ~3500 "fps" | presents, 120 fps vsync (patch 1) |
| `GL_MAX_SHADER_STORAGE_BLOCK_SIZE` | 134217728 | 1073741824 (patch 2) |
| feature suite | 32 pass / 9 fail (window-related) | 37 pass / 4 fail |
| remaining failures | — | all four are `flat`-varying draws (D3DMetal GS path, REPORTS.md §3) |
| `terrain`: 8186 indirect patch draws | 104.6 ms | 104.6 ms (D3DMetal, REPORTS.md §4) |
