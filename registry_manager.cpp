#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

// 按钮ID定义
#define ID_REGISTER_BUTTON 1001
#define ID_UNREGISTER_BUTTON 1002
#define ID_STATUS_LABEL 1003

// 窗口尺寸常量
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 220
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 35
#define BUTTON_SPACING 20

// 注册表路径 - 支持Directory和Directory\Background
const wchar_t* REGISTRY_PATH_DIR = L"Directory\\shell\\AITools";
const wchar_t* REGISTRY_PATH_BACKGROUND = L"Directory\\Background\\shell\\AITools";
const wchar_t* COMMAND_PATH_DIR = L"Directory\\shell\\AITools\\command";
const wchar_t* COMMAND_PATH_BACKGROUND = L"Directory\\Background\\shell\\AITools\\command";

// 设置为Windows子系统
#pragma comment(linker, "/subsystem:windows")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateUIState(HWND hwnd);
void ShowMessage(HWND hwnd, const wchar_t* message, BOOL isError);
BOOL RegisterContextMenu(HWND hwnd, const wchar_t* aiLauncherPath);
BOOL UnregisterContextMenu(HWND hwnd);
BOOL FindAILauncherPath(wchar_t* path, DWORD pathSize);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化通用控件
    InitCommonControls();

    // 注册窗口类
    static const wchar_t className[] = L"RegistryManager";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"注册窗口类失败!", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 计算窗口居中位置
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - WINDOW_WIDTH) / 2;
    int windowY = (screenHeight - WINDOW_HEIGHT) / 2;

    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"AI启动器右键菜单管理器",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        windowX, windowY, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL, L"创建窗口失败!", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            // 计算控件位置
            int centerX = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
            int centerY = WINDOW_HEIGHT / 2;

            // 创建状态标签
            HWND hStatusLabel = CreateWindowW(
                L"STATIC",
                L"正在检测状态...",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                50, 30, WINDOW_WIDTH - 100, 30,
                hwnd,
                (HMENU)ID_STATUS_LABEL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建注册按钮
            HWND hRegisterButton = CreateWindowW(
                L"BUTTON",
                L"注册右键菜单",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                centerX - BUTTON_WIDTH - 10, centerY - BUTTON_HEIGHT/2 - 10,
                BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_REGISTER_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建卸载按钮
            HWND hUnregisterButton = CreateWindowW(
                L"BUTTON",
                L"卸载右键菜单",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                centerX + 10, centerY - BUTTON_HEIGHT/2 - 10,
                BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_UNREGISTER_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建说明标签
            HWND hInfoLabel = CreateWindowW(
                L"STATIC",
                L"支持文件夹和目录背景右键菜单",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                50, centerY + 20,
                WINDOW_WIDTH - 100, 20,
                hwnd,
                NULL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 更新UI状态
            UpdateUIState(hwnd);
            break;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            switch (wmId) {
                case ID_REGISTER_BUTTON:
                {
                    // 查找ai_launcher.exe路径
                    wchar_t aiLauncherPath[MAX_PATH];
                    if (FindAILauncherPath(aiLauncherPath, MAX_PATH)) {
                        if (RegisterContextMenu(hwnd, aiLauncherPath)) {
                            ShowMessage(hwnd, L"右键菜单注册成功！", FALSE);
                            UpdateUIState(hwnd);
                        }
                    } else {
                        MessageBoxW(hwnd, L"未找到ai_launcher.exe文件", L"错误", MB_OK | MB_ICONERROR);
                    }
                    break;
                }

                case ID_UNREGISTER_BUTTON:
                {
                    if (UnregisterContextMenu(hwnd)) {
                        ShowMessage(hwnd, L"右键菜单卸载成功！", FALSE);
                        UpdateUIState(hwnd);
                    }
                    break;
                }
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 检查右键菜单是否已注册
BOOL IsContextMenuRegistered() {
    HKEY hKey1, hKey2;
    bool dirRegistered = false, bgRegistered = false;

    // 检查Directory注册表项
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, REGISTRY_PATH_DIR, 0, KEY_READ, &hKey1) == ERROR_SUCCESS) {
        RegCloseKey(hKey1);
        dirRegistered = true;
    }

    // 检查Directory\Background注册表项
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, REGISTRY_PATH_BACKGROUND, 0, KEY_READ, &hKey2) == ERROR_SUCCESS) {
        RegCloseKey(hKey2);
        bgRegistered = true;
    }

    return dirRegistered || bgRegistered;
}

// 查找ai_launcher.exe路径
BOOL FindAILauncherPath(wchar_t* path, DWORD pathSize) {
    // 获取当前程序所在目录
    wchar_t currentDir[MAX_PATH];
    GetModuleFileNameW(NULL, currentDir, MAX_PATH);

    // 手动移除文件名部分
    wchar_t* lastSlash = wcsrchr(currentDir, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
    }

    // 构建ai_launcher.exe路径
    wcscpy(path, currentDir);
    wcscat(path, L"\\ai_launcher.exe");

    // 检查文件是否存在
    DWORD fileAttr = GetFileAttributesW(path);
    return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

// 注册右键菜单 - 支持Directory和Directory\Background
BOOL RegisterContextMenu(HWND hwnd, const wchar_t* aiLauncherPath) {
    HKEY hKey = NULL;
    BOOL success = TRUE;
    wchar_t command[MAX_PATH + 20];
    wchar_t iconValue[MAX_PATH + 10];

    // 1. 注册到Directory（文件夹右键菜单）
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, REGISTRY_PATH_DIR, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {

        // 设置菜单项名称
        if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)L"用AI工具打开",
                          (wcslen(L"用AI工具打开") + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
            ShowMessage(hwnd, L"无法设置文件夹菜单项名称", TRUE);
            success = FALSE;
        }

        // 使用程序自身作为图标源
        GetModuleFileNameW(NULL, iconValue, MAX_PATH);
        if (RegSetValueExW(hKey, L"Icon", 0, REG_SZ, (const BYTE*)iconValue,
                          (wcslen(iconValue) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
            ShowMessage(hwnd, L"无法设置文件夹菜单图标", TRUE);
            success = FALSE;
        }

        RegCloseKey(hKey);

        // 创建command子项
        if (RegCreateKeyExW(HKEY_CLASSES_ROOT, COMMAND_PATH_DIR, 0, NULL,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            // 设置启动命令
            wcscpy(command, L"\"");
            wcscat(command, aiLauncherPath);
            wcscat(command, L"\" \"%1\"");
            if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)command,
                              (wcslen(command) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
                ShowMessage(hwnd, L"无法设置文件夹启动命令", TRUE);
                success = FALSE;
            }
            RegCloseKey(hKey);
        } else {
            ShowMessage(hwnd, L"无法创建文件夹command注册表项", TRUE);
            success = FALSE;
        }
    } else {
        ShowMessage(hwnd, L"无法创建文件夹注册表项", TRUE);
        success = FALSE;
    }

    // 2. 注册到Directory\Background（目录背景右键菜单）
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, REGISTRY_PATH_BACKGROUND, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {

        // 设置菜单项名称
        if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)L"用AI工具打开",
                          (wcslen(L"用AI工具打开") + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
            ShowMessage(hwnd, L"无法设置背景菜单项名称", TRUE);
            success = FALSE;
        }

        // 使用程序自身作为图标源
        GetModuleFileNameW(NULL, iconValue, MAX_PATH);
        if (RegSetValueExW(hKey, L"Icon", 0, REG_SZ, (const BYTE*)iconValue,
                          (wcslen(iconValue) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
            ShowMessage(hwnd, L"无法设置背景菜单图标", TRUE);
            success = FALSE;
        }

        RegCloseKey(hKey);

        // 创建command子项
        if (RegCreateKeyExW(HKEY_CLASSES_ROOT, COMMAND_PATH_BACKGROUND, 0, NULL,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            // 设置启动命令
            wcscpy(command, L"\"");
            wcscat(command, aiLauncherPath);
            wcscat(command, L"\" \"%V\"");
            if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)command,
                              (wcslen(command) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
                ShowMessage(hwnd, L"无法设置背景启动命令", TRUE);
                success = FALSE;
            }
            RegCloseKey(hKey);
        } else {
            ShowMessage(hwnd, L"无法创建背景command注册表项", TRUE);
            success = FALSE;
        }
    } else {
        ShowMessage(hwnd, L"无法创建背景注册表项", TRUE);
        success = FALSE;
    }

    // 通知系统刷新
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return success;
}

// 卸载右键菜单
BOOL UnregisterContextMenu(HWND hwnd) {
    BOOL success = TRUE;

    // 删除Directory下的command子项
    RegDeleteKeyW(HKEY_CLASSES_ROOT, COMMAND_PATH_DIR);

    // 删除Directory主项
    if (RegDeleteKeyW(HKEY_CLASSES_ROOT, REGISTRY_PATH_DIR) != ERROR_SUCCESS &&
        RegDeleteKeyW(HKEY_CLASSES_ROOT, REGISTRY_PATH_DIR) != ERROR_FILE_NOT_FOUND) {
        success = FALSE;
    }

    // 删除Directory\Background下的command子项
    RegDeleteKeyW(HKEY_CLASSES_ROOT, COMMAND_PATH_BACKGROUND);

    // 删除Directory\Background主项
    if (RegDeleteKeyW(HKEY_CLASSES_ROOT, REGISTRY_PATH_BACKGROUND) != ERROR_SUCCESS &&
        RegDeleteKeyW(HKEY_CLASSES_ROOT, REGISTRY_PATH_BACKGROUND) != ERROR_FILE_NOT_FOUND) {
        success = FALSE;
    }

    // 通知系统刷新
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return success;
}

// 更新UI状态
void UpdateUIState(HWND hwnd) {
    HWND hRegisterButton = GetDlgItem(hwnd, ID_REGISTER_BUTTON);
    HWND hUnregisterButton = GetDlgItem(hwnd, ID_UNREGISTER_BUTTON);
    HWND hStatusLabel = GetDlgItem(hwnd, ID_STATUS_LABEL);

    BOOL isRegistered = IsContextMenuRegistered();

    // 更新按钮状态
    EnableWindow(hRegisterButton, !isRegistered);
    EnableWindow(hUnregisterButton, isRegistered);

    // 更新状态标签
    if (isRegistered) {
        SetWindowTextW(hStatusLabel, L"右键菜单已注册（支持文件夹和背景右键）");
        SetWindowTextW(hRegisterButton, L"重新注册");
    } else {
        SetWindowTextW(hStatusLabel, L"右键菜单未注册");
        SetWindowTextW(hRegisterButton, L"注册右键菜单");
    }
}

// 显示消息
void ShowMessage(HWND hwnd, const wchar_t* message, BOOL isError) {
    MessageBoxW(hwnd, message, isError ? L"错误" : L"信息",
                MB_OK | (isError ? MB_ICONERROR : MB_ICONINFORMATION));
}