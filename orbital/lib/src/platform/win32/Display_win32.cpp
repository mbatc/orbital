#include "platform/Display.h"
#include "platform/Window.h"

#ifdef BFC_WINDOWS

#include <windows.h>

namespace bfc {
  // Perform our own check with RtlVerifyVersionInfo() instead of using functions from <VersionHelpers.h> as they
  // require a manifest to be functional for checks above 8.1. See https://github.com/ocornut/imgui/issues/4200
  static BOOL isWindowsVersionOrGreater(WORD major, WORD minor, WORD) {
    typedef LONG(WINAPI * PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
    static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = NULL;
    if (RtlVerifyVersionInfoFn == NULL)
      if (HMODULE ntdllModule = ::GetModuleHandleA("ntdll.dll"))
        RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
    if (RtlVerifyVersionInfoFn == NULL)
      return FALSE;
    RTL_OSVERSIONINFOEXW versionInfo = {};
    ULONGLONG conditionMask = 0;
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    versionInfo.dwMajorVersion = major;
    versionInfo.dwMinorVersion = minor;
    VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    return (RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask) == 0) ? TRUE : FALSE;
  }

  const bool isWindowsVistaOrGreater   = isWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0);      // _WIN32_WINNT_VISTA
  const bool isWindows8OrGreater       = isWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0);      // _WIN32_WINNT_WIN8
  const bool isWindows8Point1OrGreater = isWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0);    // _WIN32_WINNT_WINBLUE
  const bool isWindows10OrGreater      = isWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0);      // _WIN32_WINNT_WINTHRESHOLD / _WIN32_WINNT_WIN10

#ifndef DPI_ENUMS_DECLARED
  typedef enum { PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
  typedef enum { MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
#endif
#ifndef _DPI_AWARENESS_CONTEXTS_
  DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE (DPI_AWARENESS_CONTEXT) - 3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT) - 4
#endif
  typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                     // Shcore.lib + dll, Windows 8.1+
  typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);        // Shcore.lib + dll, Windows 8.1+
  typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)

  // Helper function to enable DPI awareness without setting up a manifest
  static int enableDpiAwareness() {
    if (isWindows10OrGreater) {
      static HINSTANCE user32_dll = ::LoadLibraryA("user32.dll"); // Reference counted per-process
      if (PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(user32_dll, "SetThreadDpiAwarenessContext")) {
        SetThreadDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        return 1;
      }
    }
    if (isWindows8Point1OrGreater) {
      static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
      if (PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore_dll, "SetProcessDpiAwareness")) {
        SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
        return 1;
      }
    }
#if _WIN32_WINNT >= 0x0600
    ::SetProcessDPIAware();
#endif

    return 1;
  }

  static float getDpiScaleForMonitor(void* monitor) {
    UINT xdpi = 96, ydpi = 96;
    if (isWindows8Point1OrGreater) {
      static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
      static PFN_GetDpiForMonitor GetDpiForMonitorFn = NULL;
      if (GetDpiForMonitorFn == NULL && shcore_dll != NULL)
        GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore_dll, "GetDpiForMonitor");
      if (GetDpiForMonitorFn != NULL) {
        GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
        return xdpi / 96.0f;
      }
    }

#ifndef NOGDI
    const HDC dc = ::GetDC(NULL);
    xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
    ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
    ::ReleaseDC(NULL, dc);
#endif
    return xdpi / 96.0f;
  }

  float getDpiScaleForHwnd(void* hwnd) {
    HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
    return getDpiScaleForMonitor(monitor);
  }

  Display fromPlatformHandle(HMONITOR hMonitor) {
    MONITORINFO wndInfo = {};
    wndInfo.cbSize = sizeof(MONITORINFO);
    if (!::GetMonitorInfo(hMonitor, &wndInfo))
      return Display();

    Display info;
    info.isPrimary = wndInfo.dwFlags & MONITORINFOF_PRIMARY;
    info.mainPos = Vec2i(wndInfo.rcMonitor.left, wndInfo.rcMonitor.top);
    info.mainSize = Vec2i(wndInfo.rcMonitor.right - wndInfo.rcMonitor.left, wndInfo.rcMonitor.bottom - wndInfo.rcMonitor.top);
    info.workPos = Vec2i(wndInfo.rcWork.left, wndInfo.rcWork.top);
    info.workSize = Vec2i(wndInfo.rcWork.right - wndInfo.rcWork.left, wndInfo.rcWork.bottom - wndInfo.rcWork.top);
    info.dpiScale = getDpiScaleForMonitor(hMonitor);
    return info;
  }

  static BOOL CALLBACK enumMonitors(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    Vector<Display>* pDisplayList = (Vector<Display>*)dwData;

    Display info = fromPlatformHandle(hMonitor);

    if (info.isPrimary)
      pDisplayList->pushFront(info);
    else
      pDisplayList->pushBack(info);

    return 1;
  }

  BFC_API Display findDisplay(platform::Window const& window) {
    HMONITOR hMonitor = ::MonitorFromWindow((HWND)window.getPlatformHandle(), MONITOR_DEFAULTTONEAREST);

    return fromPlatformHandle(hMonitor);
  }

  BFC_API Vector<Display> enumerateDisplays() {
    Vector<Display> displayList;
    ::EnumDisplayMonitors(NULL, NULL, enumMonitors, (LPARAM)&displayList);
    return displayList;
  }

  // TODO: Need ImGui to be DPI aware with multiple monitors
  // __declspec(dllexport) const int __bfc__enableDPIAwarenessHelper = enableDpiAwareness();
}

#endif
