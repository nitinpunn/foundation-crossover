/* listwin: print top-level windows (and their children) with class, title, rect, styles, owning exe */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdio.h>

static void dump(HWND h, int depth)
{
  char cls[128] = "", title[256] = "", exe[MAX_PATH] = "?";
  DWORD pid = 0; RECT r;
  GetClassNameA(h, cls, sizeof cls); GetWindowTextA(h, title, sizeof title); GetWindowRect(h, &r);
  GetWindowThreadProcessId(h, &pid);
  HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (p) { DWORD n = sizeof exe; QueryFullProcessImageNameA(p, 0, exe, &n); CloseHandle(p); }
  const char *base = strrchr(exe, '\\'); base = base ? base + 1 : exe;
  if (depth == 0 && _stricmp(base, "foundation.exe")) return;
  printf("%*shwnd=%p pid=%lu cls=\"%s\" title=\"%s\" visible=%d iconic=%d rect=%ld,%ld %ldx%ld style=0x%08lx exstyle=0x%08lx\n",
         depth * 2, "", (void *)h, pid, cls, title, IsWindowVisible(h), IsIconic(h), r.left, r.top, r.right - r.left, r.bottom - r.top,
         (unsigned long)GetWindowLongA(h, GWL_STYLE), (unsigned long)GetWindowLongA(h, GWL_EXSTYLE));
  if (depth < 2) for (HWND c = GetWindow(h, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT)) dump(c, depth + 1);
}

static BOOL CALLBACK top(HWND h, LPARAM l) { (void)l; dump(h, 0); return TRUE; }

int main(void)
{
  EnumWindows(top, 0);
  HWND fg = GetForegroundWindow(); char t[256] = ""; GetWindowTextA(fg, t, sizeof t);
  printf("foreground: %p \"%s\"\n", (void *)fg, t);
  return 0;
}
