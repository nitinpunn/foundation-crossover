/* opengl32.dll shim for Foundation: forwards everything to mesagl32.dll (Mesa) and forces
 * GL_FIRST_VERTEX_CONVENTION after each wglMakeCurrent, which sidesteps Mesa-d3d12's
 * geometry-shader provoking-vertex emulation (broken on D3DMetal; flat-varying draws vanish). */
#define WIN32_LEAN_AND_MEAN
#define WINGDIAPI
#include <windows.h>
#include <stdio.h>

typedef BOOL (WINAPI *PFN_MAKECURRENT)(HDC, HGLRC);
typedef PROC (WINAPI *PFN_GETPROC)(LPCSTR);
typedef void (APIENTRY *PFN_PROVOKING)(unsigned);
typedef HGLRC (WINAPI *PFN_GETCURRENT)(void);
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D

static HMODULE g_mesa;
static PFN_MAKECURRENT g_make_current;
static PFN_GETPROC g_get_proc;
static PFN_GETCURRENT g_get_current;
static HGLRC g_done[64]; static int g_ndone;
typedef BOOL (WINAPI *PFN_SWAP)(HDC);
static PFN_SWAP g_swap;
static char g_fps_path[MAX_PATH];
static char g_dir[MAX_PATH];
typedef void (APIENTRY *PFN_READPIXELS)(int, int, int, int, unsigned, unsigned, void *);
typedef void (APIENTRY *PFN_BINDFBO)(unsigned, unsigned);
typedef void (APIENTRY *PFN_READBUFFER)(unsigned);
typedef void (APIENTRY *PFN_PIXELSTOREI)(unsigned, int);
typedef void (APIENTRY *PFN_GETINTEGERV)(unsigned, int *);
#define GL_RGB 0x1907
#define GL_UNSIGNED_BYTE 0x1401
#define GL_BACK 0x0405
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_READ_BUFFER 0x0C02

/* on request (file shot.req next to the DLL), save the window backbuffer as shot.ppm before presenting */
static void maybe_screenshot(HDC dc)
{
  char req[MAX_PATH]; snprintf(req, sizeof req, "%sshot.req", g_dir);
  if (GetFileAttributesA(req) == INVALID_FILE_ATTRIBUTES) return;
  DeleteFileA(req);
  PFN_READPIXELS rp = (PFN_READPIXELS)GetProcAddress(g_mesa, "glReadPixels");
  PFN_READBUFFER rb = (PFN_READBUFFER)GetProcAddress(g_mesa, "glReadBuffer");
  PFN_PIXELSTOREI ps = (PFN_PIXELSTOREI)GetProcAddress(g_mesa, "glPixelStorei");
  PFN_GETINTEGERV gi = (PFN_GETINTEGERV)GetProcAddress(g_mesa, "glGetIntegerv");
  PFN_BINDFBO bf = (PFN_BINDFBO)g_get_proc("glBindFramebuffer");
  if (!rp || !rb || !ps || !gi || !bf) return;
  HWND w = WindowFromDC(dc); RECT r; GetClientRect(w, &r); int W = r.right, H = r.bottom; if (W <= 0 || H <= 0) return;
  int prev_fbo = 0, prev_rb = 0, prev_pack = 4; gi(GL_READ_FRAMEBUFFER_BINDING, &prev_fbo); gi(GL_READ_BUFFER, &prev_rb); gi(GL_PACK_ALIGNMENT, &prev_pack);
  bf(GL_READ_FRAMEBUFFER, 0); rb(GL_BACK); ps(GL_PACK_ALIGNMENT, 1);
  unsigned char *buf = malloc((size_t)W * H * 3);
  if (buf) {
    rp(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, buf);
    char out[MAX_PATH]; snprintf(out, sizeof out, "%sshot.ppm", g_dir);
    FILE *f = fopen(out, "wb");
    if (f) { fprintf(f, "P6\n%d %d\n255\n", W, H); for (int y = H - 1; y >= 0; y--) fwrite(buf + (size_t)y * W * 3, 1, (size_t)W * 3, f); fclose(f); }
    free(buf);
  }
  bf(GL_READ_FRAMEBUFFER, prev_fbo); rb(prev_rb); ps(GL_PACK_ALIGNMENT, prev_pack);
}

static void load_mesa(void)
{
  if (g_mesa) return;
  char path[MAX_PATH]; HMODULE self; 
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&load_mesa, &self);
  GetModuleFileNameA(self, path, sizeof path);
  char *slash = strrchr(path, '\\'); if (slash) strcpy(slash + 1, "mesagl32.dll");
  /* Mesa reads these when libgallium_wgl initializes; set them here so no bottle configuration is needed */
  if (!GetEnvironmentVariableA("GALLIUM_DRIVER", NULL, 0)) SetEnvironmentVariableA("GALLIUM_DRIVER", "d3d12");
  if (!GetEnvironmentVariableA("MESA_GL_VERSION_OVERRIDE", NULL, 0)) SetEnvironmentVariableA("MESA_GL_VERSION_OVERRIDE", "4.3");
  if (!GetEnvironmentVariableA("MESA_GLSL_VERSION_OVERRIDE", NULL, 0)) SetEnvironmentVariableA("MESA_GLSL_VERSION_OVERRIDE", "430");
  g_mesa = LoadLibraryA(path);
  if (!g_mesa) g_mesa = LoadLibraryA("mesagl32.dll");
  if (!g_mesa) return;
  g_make_current = (PFN_MAKECURRENT)GetProcAddress(g_mesa, "wglMakeCurrent");
  g_get_proc = (PFN_GETPROC)GetProcAddress(g_mesa, "wglGetProcAddress");
  g_get_current = (PFN_GETCURRENT)GetProcAddress(g_mesa, "wglGetCurrentContext");
  g_swap = (PFN_SWAP)GetProcAddress(g_mesa, "wglSwapBuffers");
  if (slash) { strcpy(slash + 1, "fps.log"); strcpy(g_fps_path, path); slash[1] = 0; strcpy(g_dir, path); }
}

/* ---- replace the two binary patches with hooks ----
 * 1. Pixel format: with an alpha channel, Mesa's d3d12 WGL presents through DirectComposition (stubbed in Wine ->
 *    nothing visible). Steer the game to an otherwise identical no-alpha format.
 * 2. GL_MAX_SHADER_STORAGE_BLOCK_SIZE: Gallium's default is 128 MB, the game wants 256 MB buffers and only
 *    checks the reported value (Mesa doesn't enforce it). Report 512 MB. */
typedef int (WINAPI *PFN_WCPF)(HDC, const PIXELFORMATDESCRIPTOR *);
typedef BOOL (WINAPI *PFN_WSPF)(HDC, int, const PIXELFORMATDESCRIPTOR *);
typedef int (WINAPI *PFN_WDPF)(HDC, int, UINT, PIXELFORMATDESCRIPTOR *);
typedef void (APIENTRY *PFN_GETINTV2)(unsigned, int *);
typedef void (APIENTRY *PFN_GETINT64V)(unsigned, long long *);
static PFN_WCPF r_wcpf; static PFN_WSPF r_wspf; static PFN_WDPF r_wdpf; static PFN_GETINTV2 r_getintv; static PFN_GETINT64V r_getint64v;
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#define SSBO_CAP_REPORTED (512 << 20)
static void load_pf(void) { load_mesa(); if (!g_mesa) return; if (!r_wcpf) r_wcpf = (PFN_WCPF)GetProcAddress(g_mesa, "wglChoosePixelFormat"); if (!r_wspf) r_wspf = (PFN_WSPF)GetProcAddress(g_mesa, "wglSetPixelFormat"); if (!r_wdpf) r_wdpf = (PFN_WDPF)GetProcAddress(g_mesa, "wglDescribePixelFormat"); if (!r_getintv) r_getintv = (PFN_GETINTV2)GetProcAddress(g_mesa, "glGetIntegerv"); }
static int no_alpha_equivalent(HDC dc, const PIXELFORMATDESCRIPTOR *want)
{
  PIXELFORMATDESCRIPTOR p = *want; p.cAlphaBits = 0; p.cAlphaShift = 0; p.dwFlags &= ~(DWORD)PFD_SUPPORT_COMPOSITION;
  if (p.cColorBits == 32) p.cColorBits = 24; /* Mesa's 32-bit formats all carry alpha; the 24-bit ones are RGB8 */
  if (!r_wcpf || !r_wdpf) return 0;
  int f = r_wcpf(dc, &p); PIXELFORMATDESCRIPTOR d;
  if (f > 0 && r_wdpf(dc, f, sizeof d, &d) && d.cAlphaBits == 0) return f;
  /* chooser still returned an alpha format: scan for an equivalent without alpha */
  int n = r_wdpf(dc, 1, sizeof d, NULL);
  for (int i = 1; i <= n; i++) {
    if (!r_wdpf(dc, i, sizeof d, &d)) continue;
    if (d.cAlphaBits == 0 && (d.dwFlags & PFD_SUPPORT_OPENGL) && (d.dwFlags & PFD_DRAW_TO_WINDOW) && !(d.dwFlags & PFD_SUPPORT_GDI)
        && ((d.dwFlags & PFD_DOUBLEBUFFER) != 0) == ((want->dwFlags & PFD_DOUBLEBUFFER) != 0)
        && d.cDepthBits >= want->cDepthBits && d.cStencilBits >= want->cStencilBits && d.iPixelType == PFD_TYPE_RGBA) return i;
  }
  return 0;
}
int WINAPI wglChoosePixelFormat(HDC dc, const PIXELFORMATDESCRIPTOR *pfd)
{
  load_pf(); if (!r_wcpf) return 0;
  if (pfd && pfd->cAlphaBits) { int f = no_alpha_equivalent(dc, pfd); if (f > 0) return f; }
  return r_wcpf(dc, pfd);
}
BOOL WINAPI wglSetPixelFormat(HDC dc, int fmt, const PIXELFORMATDESCRIPTOR *pfd)
{
  load_pf(); if (!r_wspf) return FALSE;
  PIXELFORMATDESCRIPTOR d; memset(&d, 0, sizeof d);
  if (r_wdpf && r_wdpf(dc, fmt, sizeof d, &d) && d.cAlphaBits) { int f = no_alpha_equivalent(dc, &d); if (f > 0) { PIXELFORMATDESCRIPTOR d2; r_wdpf(dc, f, sizeof d2, &d2); return r_wspf(dc, f, &d2); } }
  return r_wspf(dc, fmt, pfd);
}
void APIENTRY glGetIntegerv(unsigned pname, int *v)
{
  load_pf(); if (!r_getintv) return;
  r_getintv(pname, v);
  if (pname == GL_MAX_SHADER_STORAGE_BLOCK_SIZE && v && *v < SSBO_CAP_REPORTED) *v = SSBO_CAP_REPORTED;
}
static void APIENTRY h_GetInteger64v(unsigned pname, long long *v)
{
  r_getint64v(pname, v);
  if (pname == GL_MAX_SHADER_STORAGE_BLOCK_SIZE && v && *v < SSBO_CAP_REPORTED) *v = SSBO_CAP_REPORTED;
}


/* ---- indirect tessellated draws: D3DMetal's ExecuteIndirect with patches costs ~13 us per command; direct patch draws ~1 us.
 * Read the commands back and replay them as direct draws. ---- */
#define GL_PATCHES 0x000E
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#define GL_DRAW_INDIRECT_BUFFER_BINDING 0x8F43
typedef void (APIENTRY *PFN_MDEI)(unsigned, unsigned, const void *, int, int);
typedef void (APIENTRY *PFN_DEIBVBI)(unsigned, int, unsigned, const void *, int, int, unsigned);
typedef void (APIENTRY *PFN_GETBUFSUB)(unsigned, ptrdiff_t, ptrdiff_t, void *);
typedef void (APIENTRY *PFN_GETINTV)(unsigned, int *);
static PFN_MDEI g_real_mdei; static PFN_DEIBVBI g_deibvbi; static PFN_GETBUFSUB g_getbufsub; static PFN_GETINTV g_getintv;
static unsigned char *g_cmdbuf; static size_t g_cmdcap;
static long g_stat_patch_cmds, g_stat_patch_calls, g_stat_issued, g_stat_batched;
typedef void (APIENTRY *PFN_MDEBV)(unsigned, const int *, unsigned, const void *const *, int, const int *);
static PFN_MDEBV g_mdebv;
static int *g_counts, *g_basev; static const void **g_indices; static int g_batchcap;
/* whole-buffer cache of the indirect buffer: refreshed only after something could have rewritten it */
typedef void (APIENTRY *PFN_GETBUFPARAM64)(unsigned, unsigned, long long *);
static PFN_GETBUFPARAM64 g_getbufparam64;
static int g_cache_dirty = 1, g_cache_buf; static long long g_cache_size; static unsigned char *g_cache; static size_t g_cache_cap;
static long g_stat_readbacks; static double g_stat_readback_ms;
/* async mode: refresh slot k of this frame reads the staging copy made at slot k last frame (one frame of latency, no GPU stall) */
static int g_async;
/* staging copies keyed by pass identity (draw framebuffer + viewport) so a pass always reads its own previous result */
#define NKEYS 32
static struct { unsigned key; unsigned buf; long long size; int valid; } g_stg[NKEYS]; static int g_nstg;
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_VIEWPORT 0x0BA2
static int stg_slot(void)
{
  int fbo = 0, vp[4] = { 0, 0, 0, 0 }; g_getintv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo); g_getintv(GL_VIEWPORT, vp);
  unsigned key = 2166136261u; unsigned vals[5] = { (unsigned)fbo, (unsigned)vp[0], (unsigned)vp[1], (unsigned)vp[2], (unsigned)vp[3] };
  for (int i = 0; i < 5; i++) { key ^= vals[i]; key *= 16777619u; }
  for (int i = 0; i < g_nstg; i++) if (g_stg[i].key == key) return i;
  if (g_nstg < NKEYS) { g_stg[g_nstg].key = key; g_stg[g_nstg].valid = 0; g_stg[g_nstg].size = 0; return g_nstg++; }
  return -1;
}
typedef void (APIENTRY *PFN_GENBUF)(int, unsigned *); typedef void (APIENTRY *PFN_BINDBUF)(unsigned, unsigned);
static PFN_GENBUF g_genbuf; static PFN_BINDBUF g_bindbuf;
#define GL_COPY_WRITE_BUFFER 0x8F37
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_STREAM_READ 0x88E1
static double qpc_ms(void) { static LARGE_INTEGER f; LARGE_INTEGER c; if (!f.QuadPart) QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c); return (double)c.QuadPart * 1000.0 / (double)f.QuadPart; }
#define GL_BUFFER_SIZE 0x8764
typedef void (APIENTRY *PFN_V_U)(unsigned); typedef void (APIENTRY *PFN_V_UUU)(unsigned, unsigned, unsigned); typedef void (APIENTRY *PFN_V_P)(ptrdiff_t);
typedef void (APIENTRY *PFN_BUFDATA)(unsigned, ptrdiff_t, const void *, unsigned); typedef void (APIENTRY *PFN_BUFSUB)(unsigned, ptrdiff_t, ptrdiff_t, const void *);
typedef void (APIENTRY *PFN_NBUFDATA)(unsigned, ptrdiff_t, const void *, unsigned); typedef void (APIENTRY *PFN_NBUFSUB)(unsigned, ptrdiff_t, ptrdiff_t, const void *);
typedef void (APIENTRY *PFN_COPYBUF)(unsigned, unsigned, ptrdiff_t, ptrdiff_t, ptrdiff_t);
static PFN_V_U r_MemoryBarrier; static PFN_V_UUU r_DispatchCompute; static PFN_V_P r_DispatchComputeIndirect; static PFN_BUFDATA r_BufferData; static PFN_BUFSUB r_BufferSubData;
static PFN_NBUFDATA r_NamedBufferData; static PFN_NBUFSUB r_NamedBufferSubData; static PFN_COPYBUF r_CopyBufferSubData;
static void APIENTRY h_MemoryBarrier(unsigned b) { g_cache_dirty = 1; r_MemoryBarrier(b); }
static void APIENTRY h_DispatchCompute(unsigned x, unsigned y, unsigned z) { g_cache_dirty = 1; r_DispatchCompute(x, y, z); }
static void APIENTRY h_DispatchComputeIndirect(ptrdiff_t o) { g_cache_dirty = 1; r_DispatchComputeIndirect(o); }
static void APIENTRY h_BufferData(unsigned t, ptrdiff_t sz, const void *d, unsigned u) { g_cache_dirty = 1; r_BufferData(t, sz, d, u); }
static void APIENTRY h_BufferSubData(unsigned t, ptrdiff_t o, ptrdiff_t sz, const void *d) { g_cache_dirty = 1; r_BufferSubData(t, o, sz, d); }
static void APIENTRY h_NamedBufferData(unsigned b, ptrdiff_t sz, const void *d, unsigned u) { if ((int)b == g_cache_buf) g_cache_dirty = 1; r_NamedBufferData(b, sz, d, u); }
static void APIENTRY h_NamedBufferSubData(unsigned b, ptrdiff_t o, ptrdiff_t sz, const void *d) { if ((int)b == g_cache_buf) g_cache_dirty = 1; r_NamedBufferSubData(b, o, sz, d); }
static void APIENTRY h_CopyBufferSubData(unsigned rt, unsigned wt, ptrdiff_t ro, ptrdiff_t wo, ptrdiff_t sz) { g_cache_dirty = 1; r_CopyBufferSubData(rt, wt, ro, wo, sz); }

static void APIENTRY hook_MultiDrawElementsIndirect(unsigned mode, unsigned type, const void *indirect, int drawcount, int stride)
{
  if (mode != GL_PATCHES || !g_deibvbi || !g_getbufsub || drawcount <= 0) { g_real_mdei(mode, type, indirect, drawcount, stride); return; }
  int bound = 0; g_getintv(GL_DRAW_INDIRECT_BUFFER_BINDING, &bound);
  if (!bound) { g_real_mdei(mode, type, indirect, drawcount, stride); return; }
  if (stride == 0) stride = 20;
  size_t need = (size_t)stride * drawcount, off = (size_t)indirect;
  const unsigned char *cmds;
  if (g_getbufparam64) {
    if (g_cache_dirty || bound != g_cache_buf) {
      long long sz = 0; g_getbufparam64(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &sz);
      if (sz > 0 && sz <= (64 << 20)) {
        if ((size_t)sz > g_cache_cap) { free(g_cache); g_cache = malloc((size_t)sz); g_cache_cap = g_cache ? (size_t)sz : 0; }
        if (g_cache) {
          double t0 = qpc_ms();
          int k = (g_async && g_genbuf && g_bindbuf && r_CopyBufferSubData && r_BufferData) ? stg_slot() : -1;
          if (k >= 0) {
            if (!g_stg[k].buf) g_genbuf(1, &g_stg[k].buf);
            g_bindbuf(GL_COPY_WRITE_BUFFER, g_stg[k].buf);
            if (g_stg[k].size != sz) { r_BufferData(GL_COPY_WRITE_BUFFER, (ptrdiff_t)sz, NULL, GL_STREAM_READ); g_stg[k].size = sz; g_stg[k].valid = 0; }
            if (g_stg[k].valid) g_getbufsub(GL_COPY_WRITE_BUFFER, 0, (ptrdiff_t)sz, g_cache); /* previous result for this pass: already complete, no stall */
            else g_getbufsub(GL_DRAW_INDIRECT_BUFFER, 0, (ptrdiff_t)sz, g_cache); /* first time for this pass: synchronous */
            r_CopyBufferSubData(GL_DRAW_INDIRECT_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, (ptrdiff_t)sz); /* GPU-side copy of the current result for next time */
            g_stg[k].valid = 1; g_bindbuf(GL_COPY_WRITE_BUFFER, 0);
          } else g_getbufsub(GL_DRAW_INDIRECT_BUFFER, 0, (ptrdiff_t)sz, g_cache);
          g_stat_readback_ms += qpc_ms() - t0;
          g_cache_size = sz; g_cache_buf = bound; g_cache_dirty = 0; g_stat_readbacks++; }
      }
    }
    if (!g_cache_dirty && bound == g_cache_buf && off + need <= (size_t)g_cache_size) cmds = g_cache + off;
    else { g_real_mdei(mode, type, indirect, drawcount, stride); return; }
  } else {
    if (need > g_cmdcap) { free(g_cmdbuf); g_cmdbuf = malloc(need); g_cmdcap = g_cmdbuf ? need : 0; if (!g_cmdbuf) { g_real_mdei(mode, type, indirect, drawcount, stride); return; } }
    g_getbufsub(GL_DRAW_INDIRECT_BUFFER, (ptrdiff_t)off, (ptrdiff_t)need, g_cmdbuf); g_stat_readbacks++; cmds = g_cmdbuf;
  }
  size_t tsz = type == 0x1405 ? 4 : type == 0x1403 ? 2 : 1; /* GL_UNSIGNED_INT / SHORT / BYTE */
  /* batch plain commands (1 instance, baseInstance 0) into one glMultiDrawElementsBaseVertex; anything else goes one by one */
  if (g_mdebv && drawcount > g_batchcap) { free(g_counts); free(g_basev); free(g_indices); g_counts = malloc(sizeof(int) * drawcount); g_basev = malloc(sizeof(int) * drawcount); g_indices = malloc(sizeof(void *) * drawcount); g_batchcap = (g_counts && g_basev && g_indices) ? drawcount : 0; }
  int nb = 0;
  for (int i = 0; i < drawcount; i++) {
    const unsigned *c = (const unsigned *)(cmds + (size_t)i * stride); /* count, instanceCount, firstIndex, baseVertex, baseInstance */
    if (c[0] == 0 || c[1] == 0) continue;
    g_stat_issued++;
    if (g_mdebv && g_batchcap && c[1] == 1 && c[4] == 0) { g_counts[nb] = (int)c[0]; g_indices[nb] = (const void *)(size_t)(c[2] * tsz); g_basev[nb] = (int)c[3]; nb++; continue; }
    if (nb) { g_mdebv(mode, g_counts, type, g_indices, nb, g_basev); g_stat_batched += nb; nb = 0; }
    g_deibvbi(mode, (int)c[0], type, (const void *)(size_t)(c[2] * tsz), (int)c[1], (int)c[3], c[4]);
  }
  if (nb) { g_mdebv(mode, g_counts, type, g_indices, nb, g_basev); g_stat_batched += nb; }
  g_stat_patch_cmds += drawcount; g_stat_patch_calls++;
}

PROC WINAPI wglGetProcAddress(LPCSTR name)
{
  load_mesa();
  if (!g_get_proc) return NULL;
  PROC p = g_get_proc(name);
  if (p) {
    if (!strcmp(name, "glGetInteger64v")) { r_getint64v = (PFN_GETINT64V)p; return (PROC)h_GetInteger64v; }
    if (!strcmp(name, "glMemoryBarrier")) { r_MemoryBarrier = (PFN_V_U)p; return (PROC)h_MemoryBarrier; }
    if (!strcmp(name, "glDispatchCompute")) { r_DispatchCompute = (PFN_V_UUU)p; return (PROC)h_DispatchCompute; }
    if (!strcmp(name, "glDispatchComputeIndirect")) { r_DispatchComputeIndirect = (PFN_V_P)p; return (PROC)h_DispatchComputeIndirect; }
    if (!strcmp(name, "glBufferData")) { r_BufferData = (PFN_BUFDATA)p; return (PROC)h_BufferData; }
    if (!strcmp(name, "glBufferSubData")) { r_BufferSubData = (PFN_BUFSUB)p; return (PROC)h_BufferSubData; }
    if (!strcmp(name, "glNamedBufferData")) { r_NamedBufferData = (PFN_NBUFDATA)p; return (PROC)h_NamedBufferData; }
    if (!strcmp(name, "glNamedBufferSubData")) { r_NamedBufferSubData = (PFN_NBUFSUB)p; return (PROC)h_NamedBufferSubData; }
    if (!strcmp(name, "glCopyBufferSubData")) { r_CopyBufferSubData = (PFN_COPYBUF)p; return (PROC)h_CopyBufferSubData; }
  }
  if (p && !strcmp(name, "glMultiDrawElementsIndirect")) {
    if (!g_getbufparam64) g_getbufparam64 = (PFN_GETBUFPARAM64)g_get_proc("glGetBufferParameteri64v");
    if (!g_genbuf) g_genbuf = (PFN_GENBUF)g_get_proc("glGenBuffers");
    if (!g_bindbuf) g_bindbuf = (PFN_BINDBUF)g_get_proc("glBindBuffer");
    { char cfg[MAX_PATH]; snprintf(cfg, sizeof cfg, "%sshim.cfg", g_dir); FILE *f = fopen(cfg, "r"); if (f) { char line[128]; while (fgets(line, sizeof line, f)) if (!strncmp(line, "async=", 6)) g_async = atoi(line + 6); fclose(f); } }
    g_real_mdei = (PFN_MDEI)p;
    if (!g_deibvbi) g_deibvbi = (PFN_DEIBVBI)g_get_proc("glDrawElementsInstancedBaseVertexBaseInstance");
    if (!g_getbufsub) g_getbufsub = (PFN_GETBUFSUB)g_get_proc("glGetBufferSubData");
    if (!g_mdebv) g_mdebv = (PFN_MDEBV)g_get_proc("glMultiDrawElementsBaseVertex");
    if (!g_getintv) g_getintv = (PFN_GETINTV)GetProcAddress(g_mesa, "glGetIntegerv");
    if (g_deibvbi && g_getbufsub && g_getintv) return (PROC)hook_MultiDrawElementsIndirect;
  }
  return p;
}

BOOL WINAPI wglMakeCurrent(HDC dc, HGLRC rc)
{
  load_mesa();
  if (!g_make_current) return FALSE;
  BOOL ok = g_make_current(dc, rc);
  if (ok && rc && g_get_proc) {
    for (int i = 0; i < g_ndone; i++) if (g_done[i] == rc) return ok;
    PFN_PROVOKING pv = (PFN_PROVOKING)g_get_proc("glProvokingVertex");
    if (pv) { pv(GL_FIRST_VERTEX_CONVENTION); if (g_ndone < 64) g_done[g_ndone++] = rc; }
  }
  return ok;
}

/* frame counter: appends one line per second to fps.log next to the DLL (min/avg frame time too) */
BOOL WINAPI wglSwapBuffers(HDC dc)
{
  static LARGE_INTEGER freq, last_report, last_frame; static int frames; static double worst_ms;
  load_mesa();
  if (!g_swap) return FALSE;
  maybe_screenshot(dc);
  BOOL ok = g_swap(dc);

  LARGE_INTEGER now; QueryPerformanceCounter(&now);
  if (!freq.QuadPart) { QueryPerformanceFrequency(&freq); last_report = now; last_frame = now; }
  double ft = (double)(now.QuadPart - last_frame.QuadPart) * 1000.0 / (double)freq.QuadPart; last_frame = now;
  if (ft > worst_ms) worst_ms = ft;
  frames++;
  double el = (double)(now.QuadPart - last_report.QuadPart) / (double)freq.QuadPart;
  if (el >= 2.0) {
    FILE *f = fopen(g_fps_path, "a");
    if (f) { SYSTEMTIME st; GetLocalTime(&st); fprintf(f, "%02d:%02d:%02d fps=%.1f avg_ms=%.1f worst_ms=%.1f patch_mdi=%ld calls %ld cmds %ld issued %ld batched %ld readbacks %.1f ms/frame async=%d\n", st.wHour, st.wMinute, st.wSecond, frames / el, 1000.0 * el / frames, worst_ms, g_stat_patch_calls, g_stat_patch_cmds, g_stat_issued, g_stat_batched, g_stat_readbacks, g_stat_readback_ms / (frames ? frames : 1), g_async); g_stat_patch_calls = g_stat_patch_cmds = g_stat_issued = g_stat_batched = g_stat_readbacks = 0; g_stat_readback_ms = 0; fclose(f); }
    frames = 0; worst_ms = 0; last_report = now;
  }
  return ok;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) { (void)h; (void)r; if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(h); return TRUE; }
