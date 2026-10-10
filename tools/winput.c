/* winput: inject input into the Foundation window via SendInput (runs inside the Wine bottle).
 * usage: winput info | move X Y | click X Y | rclick X Y | dblclick X Y | down X Y | up X Y
 *        | drag X1 Y1 X2 Y2 | scroll X Y DELTA | key NAME... | text STRING
 * X,Y are client-area coordinates of the game window. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HWND find_game(void)
{
  HWND h = FindWindowA("HurricaneWindowClass", NULL);
  if (!h) h = FindWindowA(NULL, "Foundation");
  return h;
}

static void to_abs(HWND h, int cx, int cy, LONG *ax, LONG *ay)
{
  POINT p = { cx, cy }; ClientToScreen(h, &p);
  int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN), sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  int sx = GetSystemMetrics(SM_XVIRTUALSCREEN), sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
  *ax = (LONG)(((double)(p.x - sx) + 0.5) * 65535.0 / sw); *ay = (LONG)(((double)(p.y - sy) + 0.5) * 65535.0 / sh);
}

static void mouse(DWORD flags, LONG ax, LONG ay, DWORD data)
{
  INPUT in = { 0 }; in.type = INPUT_MOUSE; in.mi.dx = ax; in.mi.dy = ay; in.mi.mouseData = data;
  in.mi.dwFlags = flags | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK; SendInput(1, &in, sizeof in);
}
static void move_to(HWND h, int x, int y) { LONG ax, ay; to_abs(h, x, y, &ax, &ay); mouse(MOUSEEVENTF_MOVE, ax, ay, 0); Sleep(30); }
static void smooth_move(HWND h, int x1, int y1, int x2, int y2, int steps) { for (int i = 1; i <= steps; i++) { move_to(h, x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps); Sleep(8); } }

static WORD vk_from_name(const char *n)
{
  struct { const char *n; WORD vk; } t[] = { {"esc", VK_ESCAPE}, {"enter", VK_RETURN}, {"tab", VK_TAB}, {"space", VK_SPACE}, {"up", VK_UP}, {"down", VK_DOWN},
    {"left", VK_LEFT}, {"right", VK_RIGHT}, {"backspace", VK_BACK}, {"delete", VK_DELETE}, {"shift", VK_SHIFT}, {"ctrl", VK_CONTROL}, {"alt", VK_MENU},
    {"home", VK_HOME}, {"end", VK_END}, {"pgup", VK_PRIOR}, {"pgdn", VK_NEXT}, {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f5", VK_F5}, {"f9", VK_F9}, {"f11", VK_F11}, {"f12", VK_F12} };
  for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++) if (!_stricmp(n, t[i].n)) return t[i].vk;
  if (strlen(n) == 1) return (WORD)(VkKeyScanA(n[0]) & 0xff);
  return 0;
}
static void key(WORD vk, int down) { INPUT in = { 0 }; in.type = INPUT_KEYBOARD; in.ki.wVk = vk; in.ki.wScan = (WORD)MapVirtualKeyA(vk, 0); in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP; SendInput(1, &in, sizeof in); }

int main(int argc, char **argv)
{
  if (argc < 2) { fprintf(stderr, "usage\n"); return 2; }
  HWND h = find_game();
  if (!h) { printf("no game window\n"); return 1; }
  RECT r; GetClientRect(h, &r); RECT wr; GetWindowRect(h, &wr);
  if (!strcmp(argv[1], "info")) { char t[256]; GetWindowTextA(h, t, sizeof t); printf("hwnd=%p title=\"%s\" client=%ldx%ld window=%ld,%ld-%ld,%ld fg=%d\n", (void *)h, t, r.right, r.bottom, wr.left, wr.top, wr.right, wr.bottom, GetForegroundWindow() == h); return 0; }
  SetForegroundWindow(h); Sleep(50);
  const char *c = argv[1];
  if ((!strcmp(c, "move") || !strcmp(c, "click") || !strcmp(c, "rclick") || !strcmp(c, "dblclick") || !strcmp(c, "down") || !strcmp(c, "up")) && argc >= 4) {
    int x = atoi(argv[2]), y = atoi(argv[3]); POINT cur; GetCursorPos(&cur); ScreenToClient(h, &cur);
    smooth_move(h, cur.x, cur.y, x, y, 6); LONG ax, ay; to_abs(h, x, y, &ax, &ay);
    if (!strcmp(c, "click") || !strcmp(c, "dblclick")) { mouse(MOUSEEVENTF_LEFTDOWN, ax, ay, 0); Sleep(60); mouse(MOUSEEVENTF_LEFTUP, ax, ay, 0); }
    if (!strcmp(c, "dblclick")) { Sleep(80); mouse(MOUSEEVENTF_LEFTDOWN, ax, ay, 0); Sleep(60); mouse(MOUSEEVENTF_LEFTUP, ax, ay, 0); }
    if (!strcmp(c, "rclick")) { mouse(MOUSEEVENTF_RIGHTDOWN, ax, ay, 0); Sleep(60); mouse(MOUSEEVENTF_RIGHTUP, ax, ay, 0); }
    if (!strcmp(c, "down")) mouse(MOUSEEVENTF_LEFTDOWN, ax, ay, 0);
    if (!strcmp(c, "up")) mouse(MOUSEEVENTF_LEFTUP, ax, ay, 0);
  } else if (!strcmp(c, "drag") && argc >= 6) {
    int x1 = atoi(argv[2]), y1 = atoi(argv[3]), x2 = atoi(argv[4]), y2 = atoi(argv[5]); LONG ax, ay;
    move_to(h, x1, y1); to_abs(h, x1, y1, &ax, &ay); mouse(MOUSEEVENTF_LEFTDOWN, ax, ay, 0); Sleep(80);
    smooth_move(h, x1, y1, x2, y2, 20); to_abs(h, x2, y2, &ax, &ay); Sleep(80); mouse(MOUSEEVENTF_LEFTUP, ax, ay, 0);
  } else if (!strcmp(c, "mdrag") && argc >= 6) { /* middle-button drag (camera rotate) */
    int x1 = atoi(argv[2]), y1 = atoi(argv[3]), x2 = atoi(argv[4]), y2 = atoi(argv[5]); LONG ax, ay;
    move_to(h, x1, y1); to_abs(h, x1, y1, &ax, &ay); mouse(MOUSEEVENTF_MIDDLEDOWN, ax, ay, 0); Sleep(80);
    smooth_move(h, x1, y1, x2, y2, 20); to_abs(h, x2, y2, &ax, &ay); Sleep(80); mouse(MOUSEEVENTF_MIDDLEUP, ax, ay, 0);
  } else if (!strcmp(c, "scroll") && argc >= 5) {
    int x = atoi(argv[2]), y = atoi(argv[3]), d = atoi(argv[4]); move_to(h, x, y); LONG ax, ay; to_abs(h, x, y, &ax, &ay);
    int n = abs(d); for (int i = 0; i < n; i++) { mouse(MOUSEEVENTF_WHEEL, ax, ay, (DWORD)(d > 0 ? WHEEL_DELTA : -WHEEL_DELTA)); Sleep(40); }
  } else if (!strcmp(c, "key")) {
    for (int i = 2; i < argc; i++) { WORD vk = vk_from_name(argv[i]); if (!vk) { printf("unknown key %s\n", argv[i]); continue; } key(vk, 1); Sleep(60); key(vk, 0); Sleep(60); }
  } else if (!strcmp(c, "hold") && argc >= 4) { /* hold KEY for MS milliseconds (camera pan with WASD) */
    WORD vk = vk_from_name(argv[2]); int ms = atoi(argv[3]); if (vk) { key(vk, 1); Sleep(ms); key(vk, 0); }
  } else if (!strcmp(c, "text") && argc >= 3) {
    for (const char *p = argv[2]; *p; p++) { SHORT s = VkKeyScanA(*p); WORD vk = s & 0xff; int shift = (s >> 8) & 1; if (shift) key(VK_SHIFT, 1); key(vk, 1); Sleep(30); key(vk, 0); if (shift) key(VK_SHIFT, 0); Sleep(30); }
  } else { fprintf(stderr, "bad command\n"); return 2; }
  Sleep(100); return 0;
}
