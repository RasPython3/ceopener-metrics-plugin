
#include <windows.h>
#include <wchar.h>
#include "plugin.h"


#ifdef METRICS_EXPORTS
#define METRICS_API __declspec(dllexport)
#else
#define METRICS_API __declspec(dllimport)
#endif

extern int pluginRegisterTrayIconItem(HWND hCont, HMODULE hModule, PluginTrayIconItem *item);
extern void pluginUnregisterTrayIconItem(HWND hCont, int id);
extern void pluginSetTrayIcon(HWND hCont, int id, HICON hIcon);
extern int pluginRegisterTimer(HWND hCont, int interval, PluginTimerCallback callback);
extern HWND pluginUnregisterTimer(HWND hCont, int id);

HINSTANCE g_hInst = NULL;

static int g_width = 96;
static int g_height = 16;
static int g_trayId = 0;
static int g_timerId = -1;
static DWORD g_lasttick = 0;
static DWORD g_lastidle = 0;
static float g_cpu_rate = 0;
static float g_mem_rate = 0;
static int g_interval = 2000;

static HDC hbuf;
static HBITMAP hbm, hbmmsk;
static HICON hicn;
static ICONINFO icninfo;

static void onTimer(HWND hCont, int id);


static struct {
  HBRUSH black;
  HBRUSH white;
  HBRUSH cpu;
  HBRUSH mem;
} g_brushes;


METRICS_API BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
  {
    g_hInst = (HINSTANCE)hInst;
  }
  return TRUE;
}

static void InitBrush()
{
  g_brushes.black = GetStockObject(BLACK_BRUSH);
  g_brushes.white = GetStockObject(WHITE_BRUSH);
  g_brushes.cpu = CreateSolidBrush(0x000000ff);
  g_brushes.mem = CreateSolidBrush(0x0000ff00);
}

static void DeinitBrush()
{
  DeleteObject(g_brushes.black);
  DeleteObject(g_brushes.white);
  DeleteObject(g_brushes.cpu);
  DeleteObject(g_brushes.mem);
}

METRICS_API int pluginInit(HWND hCont)
{
  int i;
  HDC hdc;
  HWND hTrayWnd;
  RECT wrect;

  PluginTrayIconItem trayItem = {0};

  g_trayId = pluginRegisterTrayIconItem(hCont, g_hInst, &trayItem);
  hTrayWnd = GetDlgItem(hCont, g_trayId);

  GetWindowRect(hTrayWnd, &wrect);
  SetWindowPos(hTrayWnd, HWND_TOP, wrect.right - g_width, wrect.bottom - g_height, g_width, g_height, 0);

  hdc = GetDC(hCont);
  InitBrush();

  hbm = CreateCompatibleBitmap(hdc, g_width, g_height);
  hbmmsk = CreateCompatibleBitmap(hdc, g_width, g_height);
  hbuf = CreateCompatibleDC(hdc);


  SelectObject(hbuf, hbm);
  SelectObject(hbuf, g_brushes.black);
  PatBlt(hbuf, 0, 0, g_width, g_height, PATCOPY);
 
  SelectObject(hbuf, hbmmsk);
  PatBlt(hbuf, 0, 0, g_width, g_height, WHITENESS);

  for (i = 0; i < 2; i++) {
    int offset_y = i * (g_height / 2 - 1);
    PatBlt(hbuf, 2, offset_y + 2, g_width - 4, g_height / 2 - 3, BLACKNESS);
  }

  ReleaseDC(hCont, hdc);

  g_lasttick = GetTickCount();
  g_lastidle = GetIdleTime();

  updateTrayIcon(hCont);

  g_timerId = pluginRegisterTimer(hCont, g_interval, onTimer);

  return 1;
}

METRICS_API void pluginTerminate(HWND hCont)
{
  pluginUnregisterTimer(hCont, g_timerId);
	pluginUnregisterTrayIconItem(hCont, g_trayId);

	DestroyIcon(hicn);
  DeleteObject(hbm);
  DeleteObject(hbmmsk);
  DeleteDC(hbuf);
  DeinitBrush();
}

void updateTrayIcon(HWND hCont)
{
  int i;
  int hdcId;
  HDC hdc;
  HICON oldhicn = hicn;

  icninfo.fIcon = TRUE;
  icninfo.xHotspot = 0;
  icninfo.yHotspot = 0;
  icninfo.hbmMask = hbmmsk;
  icninfo.hbmColor = hbm;

  SelectObject(hbuf, hbm);

  for (i = 0; i < 2; i++) {
    int offset_y = i * (g_height / 2 - 1);

    SelectObject(hbuf, g_brushes.black);
    Rectangle(hbuf, 2, offset_y + 2, g_width - 2, offset_y + g_height / 2 - 1);

    SelectObject(hbuf, i == 0 ? g_brushes.cpu : g_brushes.mem);
    Rectangle(hbuf, 2, offset_y + 2, g_width - 2 - (int)((g_width - 4) * (1 - (i == 0 ? g_cpu_rate : g_mem_rate))), offset_y + g_height / 2 - 1);
  }

  hdcId = SaveDC(hbuf);
  DeleteDC(hbuf);

  hicn = CreateIconIndirect(&icninfo);

  hdc = GetDC(hCont);
  hbuf = CreateCompatibleDC(hdc);

  RestoreDC(hbuf, hdcId);
  ReleaseDC(hCont, hdc);

  pluginSetTrayIcon(hCont, g_trayId, hicn);

  DestroyIcon(oldhicn);
}

static void onTimer(HWND hCont, int id)
{
  DWORD curtick, curidle;
  MEMORYSTATUS membuf;
      
  curtick = GetTickCount();
  curidle = GetIdleTime();
  GlobalMemoryStatus(&membuf);

  if (curtick > g_lasttick && curidle >= g_lastidle) {
    g_cpu_rate = 1.0f - (float)(curidle - g_lastidle) / (float)(curtick - g_lasttick);
  }

  g_mem_rate = 1.0f - (float)(membuf.dwAvailPhys) / (float)(membuf.dwTotalPhys);

  g_lasttick = curtick;
  g_lastidle = curidle;

  updateTrayIcon(hCont);
}

