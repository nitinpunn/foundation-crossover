/* fixinstall: self-contained installer for the Foundation OpenGL fix (Mesa d3d12 + shim).
 * The payload (a simple TLV list of files) is appended to this executable at build time.
 * Installs into %SystemRoot%\system32 and registers DLL overrides for foundation.exe.
 * Options: /S silent, /U uninstall. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC "FNDFIX01"
static const char *files[] = { "opengl32.dll", "mesagl32.dll", "libgallium_wgl.dll", "dxil.dll", "shim.cfg", NULL };

static int set_overrides(int add)
{
  HKEY k; const char *path = "Software\\Wine\\AppDefaults\\foundation.exe\\DllOverrides";
  if (!add) { RegDeleteKeyA(HKEY_CURRENT_USER, path); return 1; }
  if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) != ERROR_SUCCESS) return 0;
  const char *v = "native,builtin";
  RegSetValueExA(k, "opengl32", 0, REG_SZ, (const BYTE *)v, (DWORD)strlen(v) + 1);
  RegSetValueExA(k, "dxil", 0, REG_SZ, (const BYTE *)v, (DWORD)strlen(v) + 1);
  RegCloseKey(k); return 1;
}

static void msg(int silent, const char *s, UINT flags) { if (silent) printf("%s\n", s); else MessageBoxA(NULL, s, "Foundation OpenGL fix", flags); }

int main(int argc, char **argv)
{
  int silent = 0, uninstall = 0;
  for (int i = 1; i < argc; i++) { if (!_stricmp(argv[i], "/S")) silent = 1; if (!_stricmp(argv[i], "/U")) uninstall = 1; }
  char sysdir[MAX_PATH]; GetSystemDirectoryA(sysdir, sizeof sysdir);
  if (uninstall) {
    for (int i = 0; files[i]; i++) { char p[MAX_PATH]; snprintf(p, sizeof p, "%s\\%s", sysdir, files[i]); DeleteFileA(p); }
    set_overrides(0); msg(silent, "Foundation OpenGL fix removed. Wine's own opengl32 placeholder is not restored; this is harmless.", MB_ICONINFORMATION); return 0;
  }
  char self[MAX_PATH]; GetModuleFileNameA(NULL, self, sizeof self);
  FILE *f = fopen(self, "rb"); if (!f) { msg(silent, "cannot open own executable", MB_ICONERROR); return 1; }
  fseek(f, -16, SEEK_END); char tail[16]; fread(tail, 1, 16, f);
  if (memcmp(tail + 8, MAGIC, 8)) { msg(silent, "payload missing (not a built installer)", MB_ICONERROR); return 1; }
  unsigned long long payload_size; memcpy(&payload_size, tail, 8);
  fseek(f, -(long)(16 + payload_size), SEEK_END);
  int n = 0;
  for (;;) {
    unsigned name_len = 0, size_hi = 0, size_lo = 0;
    if (fread(&name_len, 4, 1, f) != 1 || name_len == 0) break;
    char name[256]; if (name_len >= sizeof name) break; fread(name, 1, name_len, f); name[name_len] = 0;
    fread(&size_lo, 4, 1, f); fread(&size_hi, 4, 1, f); unsigned long long size = ((unsigned long long)size_hi << 32) | size_lo;
    char out[MAX_PATH]; snprintf(out, sizeof out, "%s\\%s", sysdir, name);
    FILE *o = fopen(out, "wb"); if (!o) { char m[512]; snprintf(m, sizeof m, "cannot write %s", out); msg(silent, m, MB_ICONERROR); fclose(f); return 1; }
    char buf[1 << 16]; unsigned long long left = size;
    while (left) { size_t c = left > sizeof buf ? sizeof buf : (size_t)left; size_t r = fread(buf, 1, c, f); if (!r) break; fwrite(buf, 1, r, o); left -= r; }
    fclose(o); n++;
  }
  fclose(f);
  if (!set_overrides(1)) { msg(silent, "installed files but could not write registry overrides", MB_ICONWARNING); return 1; }
  char m[256]; snprintf(m, sizeof m, "Installed %d files to %s and registered DLL overrides for foundation.exe.", n, sysdir);
  msg(silent, m, MB_ICONINFORMATION); return 0;
}
