/*
 * overlay-durchklick.exe – starts a game and makes its Qt overlay windows
 * click-through (WS_EX_LAYERED | WS_EX_TRANSPARENT).
 *
 * Background: the Funplus SDK in Foundation: Galactic Frontier puts an almost fully
 * transparent Qt window (UpdateLayeredWindow, per-pixel alpha) on top of the game.
 * Windows lets clicks through transparent pixels, Wine does not. Windows with
 * LAYERED+TRANSPARENT are skipped entirely by Wine's hit-testing.
 *
 * Usage (done by start.sh): overlay-durchklick.exe <game.exe> [arguments ...]
 * The game path may be a Unix path (/home/...), it is converted to Z:\...
 * Built without a CRT, so the Win32 declarations are written out here.
 */

typedef unsigned short WCHAR;
typedef int BOOL;
typedef unsigned int DWORD;
typedef void *HANDLE;
typedef void *HWND;
typedef long long LONG_PTR;
typedef long long LPARAM;
typedef unsigned long long SIZE_T;
typedef BOOL (__stdcall *WNDENUMPROC)(HWND, LPARAM);

typedef struct {
    DWORD cb; WCHAR *lpReserved, *lpDesktop, *lpTitle;
    DWORD dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
    unsigned short wShowWindow, cbReserved2; unsigned char *lpReserved2;
    HANDLE hStdInput, hStdOutput, hStdError;
} STARTUPINFOW;

typedef struct { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; } PROCESS_INFORMATION;

#define GWL_EXSTYLE        (-20)
#define WS_EX_TRANSPARENT  0x00000020L
#define WS_EX_LAYERED      0x00080000L
#define WAIT_TIMEOUT       258
#define GENERIC_WRITE      0x40000000
#define FILE_SHARE_READ    1
#define CREATE_ALWAYS      2
#define INVALID_HANDLE     ((HANDLE)(LONG_PTR)-1)

__declspec(dllimport) WCHAR *__stdcall GetCommandLineW(void);
__declspec(dllimport) BOOL __stdcall CreateProcessW(const WCHAR *, WCHAR *, void *, void *, BOOL, DWORD,
                                                    void *, const WCHAR *, STARTUPINFOW *, PROCESS_INFORMATION *);
__declspec(dllimport) DWORD __stdcall WaitForSingleObject(HANDLE, DWORD);
__declspec(dllimport) BOOL __stdcall GetExitCodeProcess(HANDLE, DWORD *);
__declspec(dllimport) void __stdcall ExitProcess(DWORD);
__declspec(dllimport) DWORD __stdcall GetModuleFileNameW(void *, WCHAR *, DWORD);
__declspec(dllimport) HANDLE __stdcall CreateFileW(const WCHAR *, DWORD, DWORD, void *, DWORD, DWORD, HANDLE);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE, const void *, DWORD, DWORD *, void *);
__declspec(dllimport) BOOL __stdcall FlushFileBuffers(HANDLE);
__declspec(dllimport) DWORD __stdcall GetLastError(void);

__declspec(dllimport) BOOL __stdcall EnumWindows(WNDENUMPROC, LPARAM);
__declspec(dllimport) BOOL __stdcall EnumChildWindows(HWND, WNDENUMPROC, LPARAM);
__declspec(dllimport) int __stdcall GetClassNameW(HWND, WCHAR *, int);
__declspec(dllimport) DWORD __stdcall GetWindowThreadProcessId(HWND, DWORD *);
__declspec(dllimport) LONG_PTR __stdcall GetWindowLongPtrW(HWND, int);
__declspec(dllimport) LONG_PTR __stdcall SetWindowLongPtrW(HWND, int, LONG_PTR);
__declspec(dllimport) int __cdecl wsprintfW(WCHAR *, const WCHAR *, ...);

/* clang emits memset calls for zeroing structs, so provide one without a CRT */
void *memset(void *d, int c, SIZE_T n)
{
    unsigned char *p = d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}

static HANDLE logfile = INVALID_HANDLE;
static DWORD game_pid;
static int changed;

static void logw(const WCHAR *text)
{
    char buf[1024];
    DWORD n = 0, written;
    if (logfile == INVALID_HANDLE) return;
    while (text[n] && n < sizeof(buf) - 2) { buf[n] = text[n] < 128 ? (char)text[n] : '?'; n++; }
    buf[n++] = '\n';
    WriteFile(logfile, buf, n, &written, 0);
    FlushFileBuffers(logfile);
}

static int starts_with(const WCHAR *s, const WCHAR *p)
{
    while (*p) if (*s++ != *p++) return 0;
    return 1;
}

static int ends_with(const WCHAR *s, const WCHAR *e)
{
    int ls = 0, le = 0;
    while (s[ls]) ls++;
    while (e[le]) le++;
    if (le > ls) return 0;
    return starts_with(s + ls - le, e);
}

/* Qt overlay: class Qt<version>QWindow… that is already WS_EX_LAYERED */
static void check_window(HWND hwnd)
{
    WCHAR cls[128], line[256];
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (!(ex & WS_EX_LAYERED) || (ex & WS_EX_TRANSPARENT)) return;
    if (!GetClassNameW(hwnd, cls, 128)) return;
    if (!starts_with(cls, L"Qt") || !ends_with(cls, L"QWindow")) return;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
    wsprintfW(line, L"click-through: hwnd %p class %s exstyle %08x -> %08x", hwnd, cls,
              (DWORD)ex, (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    logw(line);
    changed++;
}

static BOOL __stdcall child_cb(HWND hwnd, LPARAM unused)
{
    (void)unused;
    check_window(hwnd);
    return 1;
}

static BOOL __stdcall top_cb(HWND hwnd, LPARAM unused)
{
    DWORD pid = 0;
    (void)unused;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != game_pid) return 1;
    check_window(hwnd);
    EnumChildWindows(hwnd, child_cb, 0);
    return 1;
}

/* skips one command line argument (quoted or not) */
static WCHAR *next_arg(WCHAR *p, WCHAR **begin, WCHAR **end)
{
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') {
        *begin = ++p;
        while (*p && *p != '"') p++;
        *end = p;
        if (*p) p++;
    } else {
        *begin = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        *end = p;
    }
    return p;
}

void start(void)
{
    static WCHAR cmd[32768], dir[4096], path[4096];
    WCHAR *p, *a, *e, *rest, *z;
    WCHAR line[512];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD code = 1, n, i, last_slash = 0;

    n = GetModuleFileNameW(0, path, 4096);
    while (n && path[n - 1] != '\\') n--;
    wsprintfW(path + n, L"overlay-durchklick.log");
    logfile = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    p = next_arg(GetCommandLineW(), &a, &e);   /* own name */
    rest = next_arg(p, &a, &e);                 /* game.exe */
    if (a == e) { logw(L"no game given"); ExitProcess(2); }

    /* convert the game path to Windows form and remember its directory */
    z = cmd;
    *z++ = '"';
    if (*a == '/') { *z++ = 'Z'; *z++ = ':'; }
    for (i = 0; a + i < e; i++) {
        WCHAR c = a[i] == '/' ? '\\' : a[i];
        dir[i] = c;
        if (c == '\\') last_slash = i;
        *z++ = c;
    }
    if (*a == '/') {
        for (i = (DWORD)(e - a) + 2; i-- > 2; ) dir[i] = dir[i - 2];
        dir[0] = 'Z'; dir[1] = ':'; last_slash += 2;
    }
    dir[last_slash] = 0;
    *z++ = '"';
    while (*rest) *z++ = *rest++;
    *z = 0;

    wsprintfW(line, L"starting: %s", cmd);
    logw(line);
    wsprintfW(line, L"directory: %s", dir);
    logw(line);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(0, cmd, 0, 0, 0, 0, 0, dir, &si, &pi)) {
        wsprintfW(line, L"CreateProcess failed: %u", GetLastError());
        logw(line);
        ExitProcess(3);
    }
    game_pid = pi.dwProcessId;

    while (WaitForSingleObject(pi.hProcess, 500) == WAIT_TIMEOUT)
        EnumWindows(top_cb, 0);

    GetExitCodeProcess(pi.hProcess, &code);
    wsprintfW(line, L"game exited (code %u), %d window(s) made click-through", code, changed);
    logw(line);
    ExitProcess(code);
}
