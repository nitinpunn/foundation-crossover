# Submitting upstream — paste-ready

## A. Mesa merge request (patches 0001 + 0002)

Account: https://gitlab.freedesktop.org (sign in with GitHub works). Either:

**Option 1 — let the tooling do it** (`brew install glab`, then `glab auth login --hostname gitlab.freedesktop.org`
and tell Claude; it will fork mesa/mesa, create a branch from `main`, apply the two patches, push and open the MR).

**Option 2 — by hand**

    git clone --depth 50 https://gitlab.freedesktop.org/mesa/mesa.git && cd mesa
    git checkout -b d3d12-wgl-dcomp-fallback
    git am ../foundation-crossover/upstream/0001-*.patch ../foundation-crossover/upstream/0002-*.patch
    # fork mesa/mesa in the web UI, then:
    git remote add fork git@gitlab.freedesktop.org:<you>/mesa.git && git push -u fork d3d12-wgl-dcomp-fallback

Open the MR against `mesa/mesa:main`, label `d3d12`, title:

    d3d12: present without DirectComposition, raise max_shader_buffer_size

Description:

> Two small fixes found while running an OpenGL 4.3 game (Foundation, Steam 690830) through the d3d12
> driver on Wine/CrossOver with Apple's D3DMetal as the D3D12 implementation.
>
> 1. **d3d12/wgl: fall back to an HWND swapchain when DirectComposition is unavailable.**
>    `d3d12_wgl_framebuffer_resize()` routes any pixel format with alpha through a DirectComposition
>    visual. Where `dcomp.dll` is a stub (Wine), `dcomp_get_device()` fails and the function returns
>    early: the window never gets a swapchain, nothing is presented, and the app sees no error. The patch
>    falls back to `DXGI_ALPHA_MODE_IGNORE` + `CreateSwapChainForHwnd`. Compositing the window against the
>    desktop with alpha is lost in that case, which no GL application depends on.
> 2. **d3d12: raise `max_shader_buffer_size` to 1 GB.** The gallium default (128 MB) is reported as
>    `GL_MAX_SHADER_STORAGE_BLOCK_SIZE`; D3D12 buffers are far larger and nothing in the driver depends on
>    the lower value. The game allocates 256 MB SSBOs and refuses to run below that.
>
> Tested with a mingw cross-build of 26.2.4 on CrossOver 25.1.1 / D3DMetal (macOS 26.6): with (1) an
> alpha-8 pixel format presents normally; (2) reports 1 GB. Standalone test program and results:
> https://github.com/nitinpunn/foundation-crossover/tree/main/upstream

Mesa requires a `Reviewed-by`/CI pass; expect comments on (2) about whether a lower bound (e.g. the
D3D12 resource size expression) should be used instead of a constant.

## B. Mesa issue: driver-generated provoking-vertex GS renders nothing on D3DMetal (REPORTS.md §3)

https://gitlab.freedesktop.org/mesa/mesa/-/issues/new — title:

    d3d12: draws using flat varyings render nothing when the GS reorder variant is used (D3DMetal)

Body: paste REPORTS.md §3 plus the `glcheck` output lines; attach `tools/glcheck.c`. Ask whether an
adapter quirk (treat `last-vertex` as `first-vertex` when `load_at_vertex` is unavailable and the GS path
is known-broken) would be acceptable, since D3DMetal cannot be fixed from Mesa's side.

## C. CodeWeavers support ticket (REPORTS.md §3 + §4)

https://www.codeweavers.com/support/tickets (logged in). Product CrossOver 25.1.1 / macOS. Title:

    D3DMetal: two issues hit by Mesa's d3d12 OpenGL driver (flat-varying GS draws dropped; ExecuteIndirect+tessellation ~13 µs/cmd)

Body: one paragraph of context (Foundation, GL 4.3, Mesa-on-D3DMetal, repo link), then §3 and §4 from
REPORTS.md verbatim, and the CrossTie link. Ask them to forward to Apple and to consider the CrossTie
(stock Mesa + open-source shim) for the Foundation entry.

## D. Apple Feedback Assistant (REPORTS.md §3 + §4)

Feedback Assistant → Developer Tools & Resources → Game Porting Toolkit. One report per issue; attach
`glcheck.exe`, the `terrain`/`full` outputs, and the `d3d12_gs_variant.cpp` reference for §3. Apple
reads these; a Feedback ID is also what CodeWeavers will ask for.

## E. CodeWeavers compatibility database (the CrossTie)

Prerequisite: codeweavers.com account, then on https://www.codeweavers.com/compatibility/crossover/foundation
request to become the entry's **advocate** (sidebar link). The *CrossTie Editor* tab appears after staff
approve you. Nothing can be uploaded as a file — the editor is a form; values below mirror
`crosstie/Foundation-OpenGL-Mesa.tie`.

### E1. New entry for the component (Compatibility → *Submit App*)
- Name: `Foundation OpenGL 4.3 fix (Mesa on D3DMetal)`
- Category: Non-Applications / Components
- Description: *Provides OpenGL 4.3 to Foundation on macOS by running Mesa's OpenGL implementation (stock
  mesa-dist-win 26.2.4, MIT) on top of CrossOver's D3DMetal, through a small open-source shim. Installs
  opengl32.dll (shim), mesagl32.dll, libgallium_wgl.dll, dxil.dll and shim.cfg into the bottle's system32
  and registers DLL overrides for foundation.exe only; other programs in the bottle are unaffected.
  Source, tests and upstream patches: https://github.com/nitinpunn/foundation-crossover*

Then its CrossTie Editor → **New Application Profile**:
- CrossOver Version: `25.0` · Flag: **Component** · Bottle template: Windows 10 64-bit
- Download URL (blank locale): `https://github.com/nitinpunn/foundation-crossover/releases/download/v1.0/foundation-opengl-fix.exe`
- Advanced Install Detection → Installed File Globs: `%SystemRoot%\system32\mesagl32.dll`

**New Install Profile**:
- Installation Notes (blank locale): `Installs silently. Remove by running the installer again with /U.`
- Install Profile tab → Installer Globs: `foundation-opengl-fix*.exe`
- Advanced Installation Environment → Installer Silent Options: `/S`
- Advanced Installation Steps → Pre-Install Registry:
  key `HKEY_CURRENT_USER\Software\Wine\AppDefaults\foundation.exe\DllOverrides`,
  value `opengl32` = `native,builtin`; value `dxil` = `native,builtin`
- Post-installation → Skip Adding Icons: yes; Skip Document Associations: yes (no menu entries)

Save (parse check), **Download Tie**, test it in a fresh bottle, then **Publish**. Note the generated
`com.codeweavers.c4.NNNNN` id.

### E2. Foundation entry → CrossTie Editor
- Application Profile: CrossOver Version `25.0`, Flag Application, bottle template Windows 10 64-bit,
  Advanced → Steam Id `690830`.
- Install Profile → Pre-Dependencies: `com.codeweavers.c4.206` (Steam) and the component id from E1.
- Installation Notes: *After installation run Steam in the bottle, log in and install Foundation. Before
  the first launch copy usersetting.seed.config (from the GitHub repo) to "~/Documents/Polymorph
  Games/Foundation/usersetting.config", or set windowed mode in the game's Settings. Roughly 60 fps at
  medium settings on an M4 Max; the fix needs Apple silicon and CrossOver 25 or newer.*
- Publish → review queue. After approval the entry shows "Install Foundation via CrossTie".

### E3. In parallel (no advocate status needed)
- Forum tab of the Foundation entry: reply in "Running Foundation from Polymorph Games" with the repo
  link, the .tie, and the one-paragraph summary above.
- Rate the entry for CrossOver 25.1.1 / Mac once you've played a session with the fix ("Runs well"
  with a note that the CrossTie is required) — ratings are what move it off "known not to work".
