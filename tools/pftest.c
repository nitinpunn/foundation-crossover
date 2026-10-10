/* pftest: mimic glretrace's pixel format request via both gdi32 and opengl32 entry points, then scan which indexes SetPixelFormat accepts */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
typedef int (WINAPI *PFN_CPF)(HDC, const PIXELFORMATDESCRIPTOR *);
typedef BOOL (WINAPI *PFN_SPF)(HDC, int, const PIXELFORMATDESCRIPTOR *);
typedef int (WINAPI *PFN_DPF)(HDC, int, UINT, PIXELFORMATDESCRIPTOR *);
static HWND mkwin(void) { static int reg; WNDCLASSA wc = {0}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "pftest"; wc.style = CS_OWNDC; if (!reg) { RegisterClassA(&wc); reg = 1; } return CreateWindowA("pftest", "pf", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, NULL, NULL, wc.hInstance, NULL); }
int main(void)
{
  HMODULE gl = LoadLibraryA("OPENGL32.DLL"); char path[MAX_PATH] = "?"; GetModuleFileNameA(gl, path, sizeof path); printf("opengl32 = %s\n", path);
  PFN_CPF wcpf = (PFN_CPF)GetProcAddress(gl, "wglChoosePixelFormat"); PFN_SPF wspf = (PFN_SPF)GetProcAddress(gl, "wglSetPixelFormat"); PFN_DPF wdpf = (PFN_DPF)GetProcAddress(gl, "wglDescribePixelFormat");
  printf("exports: wglChoosePixelFormat=%p wglSetPixelFormat=%p wglDescribePixelFormat=%p\n", (void*)wcpf, (void*)wspf, (void*)wdpf);
  PIXELFORMATDESCRIPTOR pfd = {0}; pfd.nSize = sizeof pfd; pfd.nVersion = 1; pfd.iPixelType = PFD_TYPE_RGBA; pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; pfd.cColorBits = 3; pfd.cAlphaBits = 1; pfd.cDepthBits = 1; pfd.cStencilBits = 1; pfd.iLayerType = PFD_MAIN_PLANE;
  { HWND w = mkwin(); HDC dc = GetDC(w); int i = ChoosePixelFormat(dc, &pfd); PIXELFORMATDESCRIPTOR d; DescribePixelFormat(dc, i, sizeof d, &d); BOOL ok = SetPixelFormat(dc, i, &d); printf("gdi32:    Choose=%d (color %d alpha %d depth %d stencil %d flags 0x%lx) Set=%d err=%lu\n", i, d.cColorBits, d.cAlphaBits, d.cDepthBits, d.cStencilBits, d.dwFlags, ok, GetLastError()); DestroyWindow(w); }
  if (wcpf && wspf && wdpf) { HWND w = mkwin(); HDC dc = GetDC(w); int i = wcpf(dc, &pfd); PIXELFORMATDESCRIPTOR d; wdpf(dc, i, sizeof d, &d); BOOL ok = wspf(dc, i, &d); printf("opengl32: Choose=%d (color %d alpha %d depth %d stencil %d flags 0x%lx) Set=%d err=%lu\n", i, d.cColorBits, d.cAlphaBits, d.cDepthBits, d.cStencilBits, d.dwFlags, ok, GetLastError()); DestroyWindow(w); }
  /* scan */
  HWND w0 = mkwin(); HDC dc0 = GetDC(w0); int n = DescribePixelFormat(dc0, 1, sizeof pfd, NULL); int nw = wdpf ? wdpf(dc0, 1, sizeof pfd, NULL) : -1; printf("format counts: gdi32=%d opengl32=%d\n", n, nw); DestroyWindow(w0);
  int max = nw > n ? nw : n; char acc_g[512] = "", acc_w[512] = ""; int cg = 0, cw = 0;
  for (int i = 1; i <= max && i < 300; i++) {
    PIXELFORMATDESCRIPTOR d; HWND w = mkwin(); HDC dc = GetDC(w); DescribePixelFormat(dc, i, sizeof d, &d); if (SetPixelFormat(dc, i, &d)) { cg++; if (strlen(acc_g) < 480) sprintf(acc_g + strlen(acc_g), "%d ", i); } DestroyWindow(w);
    if (wspf) { w = mkwin(); dc = GetDC(w); wdpf(dc, i, sizeof d, &d); if (wspf(dc, i, &d)) { cw++; if (strlen(acc_w) < 480) sprintf(acc_w + strlen(acc_w), "%d ", i); } DestroyWindow(w); }
  }
  printf("gdi32 SetPixelFormat accepts %d: %s\nopengl32 wglSetPixelFormat accepts %d: %s\n", cg, acc_g, cw, acc_w);
  return 0;
}
