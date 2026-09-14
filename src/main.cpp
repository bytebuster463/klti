#include "resource.h"
#include <windows.h>
#include <shellapi.h>

#include <iostream>

constexpr wchar_t WINDOW_CLASS_NAME[] =
  L"KLTI.KeyboardLayoutTrayIcon";

constexpr wchar_t INSTANCE_MUTEX_NAME[] =
  L"KLTI.KeyboardLayoutTrayIcon.Instance";

constexpr wchar_t INSTANCE_MESSAGE_NAME[] =
  L"KLTI.KeyboardLayoutTrayIcon.Exit";

#ifndef APP_VERSION
#define APP_VERSION L"0.1 (Development Build)"
#endif
constexpr wchar_t VERSION[] = APP_VERSION;

/// App icon resource ID
#define IDI_KLTI 101

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT TIMER_LAYOUT = 1;

constexpr UINT ID_TRAY_ABOUT = 1001;
constexpr UINT ID_TRAY_EXIT = 1002;

constexpr UINT LAYOUT_TIMER_INTERVAL = 250;
constexpr UINT MAX_LAYOUTS = 8;

HINSTANCE g_instance = nullptr;
HWND g_window = nullptr;

NOTIFYICONDATAW g_tray = {};
HICON g_currentIcon = nullptr;

wchar_t g_iconDirectory[MAX_PATH];
wchar_t g_unknownIcon[MAX_PATH];

WORD g_currentLangId = 0;
bool g_haveCurrentLangId = false;

UINT g_exitMessage = 0;

#ifdef _DEBUG
void DebugLog(const wchar_t *message) {
  std::wcout << message << std::endl;
}
#endif

bool IconPathForLangId(
    WORD langId,
    wchar_t *path,
    size_t pathSize) {
  wchar_t name[32];

  swprintf_s(name, _countof(name),
      L"0x%04X.ico",
      static_cast<unsigned int>(langId));

  return wcscpy_s(path, pathSize, g_iconDirectory) == 0 &&
         wcscat_s(path, pathSize, L"\\") == 0 &&
         wcscat_s(path, pathSize, name) == 0;
}

bool DecimalIconPathForLangId(
  WORD langId,
  wchar_t* path,
  size_t pathSize)
{
  wchar_t name[32];

  swprintf_s(name, _countof(name),
    L"%u.ico",
    static_cast<unsigned int>(langId));

  return wcscpy_s(path, pathSize, g_iconDirectory) == 0 &&
         wcscat_s(path, pathSize, L"\\") == 0 &&
         wcscat_s(path, pathSize, name) == 0;
}

HICON LoadIconFromFile(const wchar_t* path) {
  return static_cast<HICON>(LoadImageW(nullptr, path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE));
}

void DestroyCurrentIcon() {
  if (g_currentIcon != nullptr) {
    DestroyIcon(g_currentIcon);
    g_currentIcon = nullptr;
  }
}

bool SetTrayIcon(HICON icon) {
  if (icon == nullptr) {
    return false;
  }

  g_tray.hIcon = icon;

  if (!Shell_NotifyIconW(NIM_MODIFY, &g_tray)) {
    return false;
  }

  DestroyCurrentIcon();
  g_currentIcon = icon;

  return true;
}

bool SetTrayIconForLangId(WORD langId) {
  wchar_t specificIconPath[MAX_PATH];

  if (!IconPathForLangId(langId, specificIconPath, _countof(specificIconPath))) { 
    return false;
  }

  HICON icon = LoadIconFromFile(specificIconPath);

  if (icon != nullptr) {
#ifdef _DEBUG
    DebugLog(specificIconPath);
#endif
    return SetTrayIcon(icon);
  }

  wchar_t decimalIconPath[MAX_PATH];

  if (!DecimalIconPathForLangId(langId, decimalIconPath, _countof(decimalIconPath))) {
    return false;
  }

  icon = LoadIconFromFile(decimalIconPath);

  if (icon != nullptr) {
#ifdef _DEBUG
    DebugLog(decimalIconPath);
#endif

    return SetTrayIcon(icon);
  }

#ifdef _DEBUG
  wchar_t message[256];

  swprintf_s(message, _countof(message), L"Icon not found: %s", specificIconPath);

  DebugLog(message);
#endif

  HICON unknownIcon = LoadIconFromFile(g_unknownIcon);

  if (unknownIcon != nullptr) {
#ifdef _DEBUG
    swprintf_s(message, _countof(message),
               L"Using unknown icon for LANGID 0x%04X", static_cast<unsigned int>(langId));

    DebugLog(message);
#endif

    return SetTrayIcon(unknownIcon);
  }

#ifdef _DEBUG
  DebugLog(L"Unable to load unknown icon. Keeping current icon.");
#endif

  return false;
}

bool GetForegroundLangId(WORD &langId) {
  HWND foregroundWindow = GetForegroundWindow();

  if (foregroundWindow == nullptr) {
    return false;
  }

  DWORD threadId = GetWindowThreadProcessId(foregroundWindow, nullptr);

  if (threadId == 0) {
    return false;
  }

  HKL keyboardLayout = GetKeyboardLayout(threadId);

  if (keyboardLayout == nullptr) {
    return false;
  }

  langId = LOWORD(reinterpret_cast<ULONG_PTR>(keyboardLayout));

  return true;
}

void UpdateCurrentLayout() {
  WORD langId = 0;

  if (!GetForegroundLangId(langId)) {
#ifdef _DEBUG
    DebugLog(L"Unable to determine foreground keyboard layout.");
#endif

    return;
  }

  if (g_haveCurrentLangId &&
      langId == g_currentLangId) {
    return;
  }

  g_currentLangId = langId;
  g_haveCurrentLangId = true;

#ifdef _DEBUG
  wchar_t message[256];

  swprintf_s(
      message,
      _countof(message),
      L"Current LANGID: %u (0x%04X)",
      static_cast<unsigned int>(langId),
      static_cast<unsigned int>(langId));

  DebugLog(message);
#endif

  SetTrayIconForLangId(langId);
}

bool FileExists(const wchar_t *path) {
  DWORD attributes = GetFileAttributesW(path);

  return attributes != INVALID_FILE_ATTRIBUTES &&
         (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool CheckInstalledIcons() {
  HKL layouts[MAX_LAYOUTS];

  UINT actualCount = GetKeyboardLayoutList(MAX_LAYOUTS, layouts);

  if (actualCount == 0) {
    #ifdef _DEBUG
      DebugLog(
        L"GetKeyboardLayoutList returned no layouts.");
    #endif

    return true;
  }

  WORD langIds[MAX_LAYOUTS];
  UINT langIdCount = 0;

  for (UINT i = 0; i < actualCount; ++i) {
    WORD langId = LOWORD(reinterpret_cast<ULONG_PTR>(layouts[i]));

    bool alreadyPresent = false;

    for (UINT j = 0; j < langIdCount; ++j) {
      if (langIds[j] == langId) {
        alreadyPresent = true;
        break;
      }
    }

    if (!alreadyPresent) {
      langIds[langIdCount++] = langId;
    }
  }

  wchar_t message[4096] =
    L"The following keyboard layout icons are missing:\n\n";

  size_t messageLength = wcslen(message);
  bool haveMissingIcons = false;

  for (UINT i = 0; i < langIdCount; ++i) {
    WORD langId = langIds[i];

    wchar_t iconPath[MAX_PATH];
    wchar_t decimalIconPath[MAX_PATH];

    if (!IconPathForLangId(langId, iconPath, _countof(iconPath))) {
      continue;
    }

    if (!DecimalIconPathForLangId(langId, decimalIconPath, _countof(decimalIconPath))) {
      continue;
    }

#ifdef _DEBUG
    wchar_t debugMessage[512];

    swprintf_s(debugMessage, _countof(debugMessage),
        L"Installed LANGID %u (0x%04X): %s",
        static_cast<unsigned int>(langId),
        static_cast<unsigned int>(langId),
        iconPath);

    DebugLog(debugMessage);
#endif

    if (!FileExists(iconPath) &&
        !FileExists(decimalIconPath)) {
      haveMissingIcons = true;

      wchar_t line[256];

      swprintf_s(line, _countof(line),
                 L"  0x%04X.ico\n",
                 static_cast<unsigned int>(langId));

      size_t lineLength = wcslen(line);

      if (messageLength + lineLength + 1 <
          _countof(message)) {
        wcscpy_s(message + messageLength, _countof(message) - messageLength, line);

        messageLength += lineLength;
      }
    }
  }

  if (!haveMissingIcons) {
    return true;
  }

  const wchar_t suffix[] =
      L"\nThe application will use unknown.ico "
      L"for these layouts.";

  if (messageLength + wcslen(suffix) + 1 <
      _countof(message)) {
    wcscpy_s(message + messageLength, _countof(message) - messageLength, suffix);
  }

  MessageBoxW(nullptr, message, L"KLTI - Missing Icons", MB_OK | MB_ICONWARNING);

  return true;
}

void ShowAboutDialog(HWND owner, HINSTANCE instance) {
  wchar_t message[256];

  swprintf_s(
      message,
      L"KLTI\n"
      L"Keyboard Layout Tray Icon\n\n"
      L"Version %ls\n"
      L"Author: bytebuster",
      VERSION);

  MSGBOXPARAMS params = {};
  params.cbSize = sizeof(params);
  params.hwndOwner = owner;
  params.hInstance = instance;
  params.lpszText = message;
  params.lpszCaption = L"About KLTI";
  params.dwStyle = MB_OK | MB_USERICON;
  params.lpszIcon = MAKEINTRESOURCEW(IDI_KLTI);

  MessageBoxIndirectW(&params);
}

void ShowTrayMenu(HWND window) {
  POINT point = {};
  GetCursorPos(&point);

  HMENU menu = CreatePopupMenu();

  if (menu == nullptr) {
    return;
  }

  AppendMenuW(menu, MF_STRING, ID_TRAY_ABOUT, L"About");

  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

  AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");

  SetForegroundWindow(window);

  UINT command = TrackPopupMenu(
      menu,
      TPM_RETURNCMD |
          TPM_NONOTIFY |
          TPM_RIGHTBUTTON,
      point.x,
      point.y,
      0,
      window,
      nullptr);

  DestroyMenu(menu);

  switch (command) {
  case ID_TRAY_ABOUT:
    ShowAboutDialog(window, g_instance);
    break;

  case ID_TRAY_EXIT:
    PostMessageW(window, WM_CLOSE, 0, 0);
    break;

  default:
    break;
  }
}

LRESULT CALLBACK WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
  if (message == g_exitMessage) {
    PostMessageW(window, WM_CLOSE, 0, 0);

    return 0;
  }

  switch (message) {
  case WM_TIMER:
    if (wParam == TIMER_LAYOUT) {
      UpdateCurrentLayout();
    }

    return 0;

  case WM_TRAYICON:
    if (lParam == WM_RBUTTONUP) {
      ShowTrayMenu(window);
    } else if (lParam == WM_LBUTTONDBLCLK) {
      ShowAboutDialog(window, g_instance);
    }

    return 0;

  case WM_CLOSE:
    DestroyWindow(window);
    return 0;

  case WM_DESTROY:
    KillTimer(window, TIMER_LAYOUT);

    Shell_NotifyIconW(NIM_DELETE, &g_tray);

    DestroyCurrentIcon();

    PostQuitMessage(0);
    return 0;

  default:
    return DefWindowProcW(window, message, wParam, lParam);
  }
}

bool CreateTrayWindow() {
  WNDCLASSW windowClass = {};
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = g_instance;
  windowClass.lpszClassName = WINDOW_CLASS_NAME;
  windowClass.hIcon = LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_KLTI));

  if (RegisterClassW(&windowClass) == 0) {
    return false;
  }

  g_window = CreateWindowExW(
      0,
      WINDOW_CLASS_NAME,
      L"KLTI",
      0,
      0,
      0,
      0,
      0,
      HWND_MESSAGE,
      nullptr,
      g_instance,
      nullptr);

  return g_window != nullptr;
}

bool CreateTrayIcon() {
  HICON icon = LoadIconFromFile(g_unknownIcon);

  if (icon == nullptr) {
    MessageBoxW(
        nullptr,
        L"Unable to load required icon:\n\n"
        L"icons\\unknown.ico",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return false;
  }

  g_currentIcon = icon;

  g_tray.cbSize = sizeof(g_tray);
  g_tray.hWnd = g_window;
  g_tray.uID = 1;
  g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  g_tray.uCallbackMessage = WM_TRAYICON;
  g_tray.hIcon = g_currentIcon;

  wcscpy_s(g_tray.szTip, L"KLTI - Keyboard Layout Tray Icon");

  if (!Shell_NotifyIconW(NIM_ADD, &g_tray)) {
    DestroyCurrentIcon();
    return false;
  }

  return true;
}

#ifdef _DEBUG

void InitializeDebugConsole() {
  if (!AllocConsole()) {
    return;
  }

  FILE *stream = nullptr;

  freopen_s(&stream, "CONOUT$", "w", stdout);

  freopen_s(&stream, "CONOUT$", "w", stderr);

  freopen_s(&stream, "CONIN$", "r", stdin);

  std::wcout.clear();
  std::wcerr.clear();
  std::wcin.clear();

  DebugLog(L"KLTI debug build");
}

#endif

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int) {
  g_instance = instance;

#ifdef _DEBUG
  InitializeDebugConsole();
#endif

  HANDLE instanceMutex = CreateMutexW(nullptr, FALSE, INSTANCE_MUTEX_NAME);

  if (instanceMutex == nullptr) {
    MessageBoxW(
        nullptr,
        L"Unable to create instance mutex.",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    UINT exitMessage = RegisterWindowMessageW(INSTANCE_MESSAGE_NAME);

    HWND existingWindow = FindWindowExW(
        HWND_MESSAGE,
        nullptr,
        WINDOW_CLASS_NAME,
        nullptr);

    if (existingWindow != nullptr) {
      PostMessageW(existingWindow, exitMessage, 0, 0);
    }

    CloseHandle(instanceMutex);

    return 0;
  }

  g_exitMessage = RegisterWindowMessageW(INSTANCE_MESSAGE_NAME);

  if (g_exitMessage == 0) {
    CloseHandle(instanceMutex);

    MessageBoxW(
        nullptr,
        L"Unable to register instance message.",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  wchar_t modulePath[MAX_PATH];

  DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

  if (length == 0 || length >= MAX_PATH) {
    CloseHandle(instanceMutex);

    MessageBoxW(
        nullptr,
        L"Unable to determine executable path.",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  wchar_t *separator = wcsrchr(modulePath, L'\\');

  if (separator == nullptr) {
    CloseHandle(instanceMutex);

    MessageBoxW(
        nullptr,
        L"Unable to determine executable directory.",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  *separator = L'\0';

  if (wcscpy_s(g_iconDirectory, _countof(g_iconDirectory), modulePath) != 0 ||
      wcscat_s(g_iconDirectory, _countof(g_iconDirectory), L"\\icons") != 0 ||
      wcscpy_s(g_unknownIcon, _countof(g_unknownIcon), g_iconDirectory) != 0 ||
      wcscat_s(g_unknownIcon, _countof(g_unknownIcon), L"\\unknown.ico") != 0) {
    CloseHandle(instanceMutex);

    MessageBoxW(
      nullptr,
      L"Unable to construct icon path.",
      L"KLTI - Fatal Error",
      MB_OK | MB_ICONERROR);

    return 1;
  }

#ifdef _DEBUG
  DebugLog(L"Icon directory:");
  DebugLog(g_iconDirectory);
#endif

  if (!FileExists(g_unknownIcon)) {
    CloseHandle(instanceMutex);

    MessageBoxW(
        nullptr,
        L"Required icon was not found:\n\n"
        L"icons\\unknown.ico",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  CheckInstalledIcons();

  if (!CreateTrayWindow()) {
    CloseHandle(instanceMutex);

    MessageBoxW(
        nullptr,
        L"Unable to create KLTI window.",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  if (!CreateTrayIcon()) {
    DestroyWindow(g_window);
    CloseHandle(instanceMutex);

    MessageBoxW(
        nullptr,
        L"Unable to create tray icon.",
        L"KLTI - Fatal Error",
        MB_OK | MB_ICONERROR);

    return 1;
  }

  UpdateCurrentLayout();

  SetTimer(g_window, TIMER_LAYOUT, LAYOUT_TIMER_INTERVAL, nullptr);

  MSG message = {};

  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }

  CloseHandle(instanceMutex);

  return static_cast<int>(message.wParam);
}
