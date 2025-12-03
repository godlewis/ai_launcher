#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windowsx.h>

// 按钮ID定义
#define ID_REGISTER_BUTTON 1001
#define ID_UNREGISTER_BUTTON 1002
#define ID_STATUS_LABEL 1003
#define ID_TERMINAL_EDIT 1004
#define ID_BROWSE_BUTTON 1005
#define ID_EXIT_BUTTON 1006
#define ID_INSTALL_BUTTON 1007  // 安装AI工具按钮

// 窗口尺寸常量
#define WINDOW_WIDTH 450
#define WINDOW_HEIGHT 240
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 35
#define BUTTON_SPACING 20

// 终端配置注册表路径
const wchar_t* TERMINAL_CONFIG_PATH = L"Software\\AILauncher";


// 注册表路径 - 支持Directory和Directory\Background
const wchar_t* REGISTRY_PATH_DIR = L"Directory\\shell\\AITools";
const wchar_t* REGISTRY_PATH_BACKGROUND = L"Directory\\Background\\shell\\AITools";
const wchar_t* COMMAND_PATH_DIR = L"Directory\\shell\\AITools\\command";
const wchar_t* COMMAND_PATH_BACKGROUND = L"Directory\\Background\\shell\\AITools\\command";

// 设置为Windows子系统
#pragma comment(linker, "/subsystem:windows")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// 全局变量 - 简化后不再需要终端检测
// TerminalInfo g_terminals[10];
// int g_terminalCount = 0;
// BOOL g_detectionCompleted = FALSE;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateUIState(HWND hwnd);
void ShowMessage(HWND hwnd, const wchar_t* message, BOOL isError);
BOOL RegisterContextMenu(HWND hwnd, const wchar_t* aiLauncherPath);
BOOL UnregisterContextMenu(HWND hwnd);
BOOL FindAILauncherPath(wchar_t* path, DWORD pathSize);

// 终端配置相关函数
BOOL ValidateTerminal(const wchar_t* terminalPath);
BOOL SaveTerminalConfig(const wchar_t* terminalPath, const wchar_t* terminalName);
BOOL LoadTerminalConfig(wchar_t* terminalPath, DWORD pathSize, wchar_t* terminalName, DWORD nameSize);
BOOL BrowseForTerminal(HWND hwnd, wchar_t* selectedPath, DWORD pathSize);

// AI工具安装向导相关函数
void ShowInstallationWizard(HWND hwnd);
BOOL IsNodeJsInstalled();
void CopyToClipboard(const wchar_t* text);
void ExecuteNpmInstall(HWND hwnd, const wchar_t* packageName);
LRESULT CALLBACK InstallWizardProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

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

// AI工具安装向导相关实现

// 定义工具信息结构
struct ToolInstallInfo {
    const wchar_t* name;
    const wchar_t* installCommand;
    int copyButtonId;
    int installButtonId;
};

// 7个AI工具的安装信息
ToolInstallInfo g_toolsInfo[] = {
    {L"Claude", L"npm install -g @anthropic-ai/claude-code", 2001, 2101},
    {L"Qwen", L"npm install -g @qwen-code/qwen-code@latest", 2002, 2102},
    {L"Codex", L"npm install -g @openai/codex", 2003, 2103},
    {L"OpenCode", L"npm install -g opencode-ai", 2004, 2104},
    {L"Gemini", L"npm install -g @google/gemini-cli", 2005, 2105},
    {L"Crush", L"npm install -g @charmland/crush", 2006, 2106},
    {L"iflow", L"npm install -g @iflow/cli", 2007, 2107}
};

// 显示AI工具安装向导
void ShowInstallationWizard(HWND hwndParent) {
    // 注册对话框类
    static const wchar_t wizardClassName[] = L"InstallWizard";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = InstallWizardProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = wizardClassName;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    wc.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
    
    if (!GetClassInfoExW(GetModuleHandle(NULL), wizardClassName, &wc)) {
        RegisterClassExW(&wc);
    }
    
    // 计算对话框尺寸和位置 - 增加宽度以留出右边距
    const int dialogWidth = 560; // 从500增加到560，留出右边距
    const int dialogHeight = 500; // 7个工具 × 60像素 + 标题栏和边距
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    // 创建对话框
    HWND hwndWizard = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
        wizardClassName,
        L"AI工具安装向导",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        dialogX, dialogY, dialogWidth, dialogHeight,
        hwndParent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!hwndWizard) {
        MessageBoxW(hwndParent, L"无法创建安装向导对话框", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    ShowWindow(hwndWizard, SW_SHOW);
    UpdateWindow(hwndWizard);
    
    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hwndWizard, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!IsWindow(hwndWizard)) break;
    }
}

// 安装向导对话框过程
LRESULT CALLBACK InstallWizardProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            // 创建工具列表
            int yPos = 30;
            const int itemHeight = 60;
            const int labelWidth = 80;
            const int commandWidth = 250;
            const int buttonWidth = 60;
            const int spacing = 10;
            const int rightMargin = 30; // 右边距，让按钮与窗口边缘保持间距
            
            for (int i = 0; i < 7; i++) {
                // 工具名称标签
                CreateWindowW(
                    L"STATIC",
                    g_toolsInfo[i].name,
                    WS_CHILD | WS_VISIBLE,
                    20, yPos + 15, labelWidth, 25,
                    hwnd,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );
                
                // 安装命令文本框(只读)
                CreateWindowW(
                    L"EDIT",
                    g_toolsInfo[i].installCommand,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                    20 + labelWidth + spacing, yPos + 15, commandWidth, 25,
                    hwnd,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );
                
                // 复制按钮
                CreateWindowW(
                    L"BUTTON",
                    L"复制",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    20 + labelWidth + spacing + commandWidth + spacing, yPos + 15, buttonWidth, 25,
                    hwnd,
                    (HMENU)g_toolsInfo[i].copyButtonId,
                    GetModuleHandle(NULL),
                    NULL
                );
                
                // 安装按钮（位置计算时考虑右边距）
                CreateWindowW(
                    L"BUTTON",
                    L"安装",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    20 + labelWidth + spacing + commandWidth + spacing + buttonWidth + spacing, yPos + 15, buttonWidth, 25,
                    hwnd,
                    (HMENU)g_toolsInfo[i].installButtonId,
                    GetModuleHandle(NULL),
                    NULL
                );
                
                yPos += itemHeight;
            }
            break;
        }
        
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            
            // 查找对应的工具
            for (int i = 0; i < 7; i++) {
                if (wmId == g_toolsInfo[i].copyButtonId) {
                    // 复制安装命令到剪贴板
                    CopyToClipboard(g_toolsInfo[i].installCommand);
                    MessageBoxW(hwnd, L"已复制到剪贴板", L"提示", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                
                if (wmId == g_toolsInfo[i].installButtonId) {
                    // 执行安装命令
                    ExecuteNpmInstall(hwnd, g_toolsInfo[i].installCommand);
                    return 0;
                }
            }
            break;
        }
        
        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
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

// 检测Node.js是否安装
BOOL IsNodeJsInstalled() {
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    // 执行node --version命令
    BOOL result = CreateProcessW(
        NULL,
        L"node --version",
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );
    
    if (result) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0);
    }
    
    return FALSE;
}

// 复制文本到剪贴板
void CopyToClipboard(const wchar_t* text) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        
        size_t len = wcslen(text) + 1;
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
        if (hGlobal) {
            wchar_t* pGlobal = (wchar_t*)GlobalLock(hGlobal);
            if (pGlobal) {
                wcscpy(pGlobal, text);
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        CloseClipboard();
    }
}

// 执行npm安装命令
void ExecuteNpmInstall(HWND hwnd, const wchar_t* installCommand) {
    // 检查Node.js是否安装
    if (!IsNodeJsInstalled()) {
        MessageBoxW(hwnd, 
            L"请先安装Node.js环境\n\n访问 https://nodejs.org/ 下载并安装Node.js", 
            L"环境检查失败", 
            MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 使用cmd.exe执行安装命令
    wchar_t command[2048];
    wsprintfW(command, L"/k %s", installCommand);
    
    HINSTANCE result = ShellExecuteW(
        NULL,
        L"open",
        L"cmd.exe",
        command,
        NULL,
        SW_SHOWNORMAL
    );
    
    if ((int)result <= 32) {
        MessageBoxW(hwnd, L"无法打开终端执行安装命令", L"错误", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            // 创建状态标签
            HWND hStatusLabel = CreateWindowW(
                L"STATIC",
                L"正在检测状态...",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                50, 15, WINDOW_WIDTH - 100, 25,
                hwnd,
                (HMENU)ID_STATUS_LABEL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建注册和卸载按钮
            HWND hRegisterButton = CreateWindowW(
                L"BUTTON",
                L"注册右键菜单",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                50, 50, BUTTON_WIDTH - 10, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_REGISTER_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            HWND hUnregisterButton = CreateWindowW(
                L"BUTTON",
                L"卸载右键菜单",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                180, 50, BUTTON_WIDTH - 10, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_UNREGISTER_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建退出按钮
            HWND hExitButton = CreateWindowW(
                L"BUTTON",
                L"退出",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                310, 50, 60, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_EXIT_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建安装AI工具按钮
            HWND hInstallButton = CreateWindowW(
                L"BUTTON",
                L"安装AI工具",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                (WINDOW_WIDTH - BUTTON_WIDTH) / 2, 95, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_INSTALL_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建说明标签
            HWND hInfoLabel = CreateWindowW(
                L"STATIC",
                L"支持文件夹和目录背景右键菜单",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                50, 85, WINDOW_WIDTH - 100, 20,
                hwnd,
                NULL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建终端选择分组框
            HWND hTerminalGroup = CreateWindowW(
                L"BUTTON",
                L"终端程序选择",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                30, 120, WINDOW_WIDTH - 60, 100,
                hwnd,
                (HMENU)ID_STATUS_LABEL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建终端选择文本框标签
            CreateWindowW(
                L"STATIC",
                L"终端程序路径:",
                WS_CHILD | WS_VISIBLE,
                50, 145, 120, 20,
                hwnd,
                NULL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建终端路径文本框
            HWND hTerminalEdit = CreateWindowW(
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                50, 170, 290, 25,
                hwnd,
                (HMENU)ID_TERMINAL_EDIT,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建浏览按钮
            HWND hBrowseButton = CreateWindowW(
                L"BUTTON",
                L"浏览...",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                345, 170, 60, 25,
                hwnd,
                (HMENU)ID_BROWSE_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

    
          // 加载当前终端配置
            wchar_t currentPath[MAX_PATH] = L"";
            wchar_t currentName[256] = L"";
            LoadTerminalConfig(currentPath, MAX_PATH, currentName, 256);

            // 设置文本框内容
            SetWindowTextW(hTerminalEdit, currentPath);

            // 更新UI状态
            UpdateUIState(hwnd);
            break;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            HWND hControl = (HWND)lParam;

            switch (wmId) {
                case ID_REGISTER_BUTTON:
                {
                    // 获取文本框中的终端路径
                    HWND hEdit = GetDlgItem(hwnd, ID_TERMINAL_EDIT);
                    wchar_t terminalPath[MAX_PATH];
                    GetWindowTextW(hEdit, terminalPath, MAX_PATH);

                    // 如果路径为空，使用默认cmd.exe
                    if (wcslen(terminalPath) == 0) {
                        wcscpy(terminalPath, L"C:\\Windows\\System32\\cmd.exe");
                    }

                    // 验证终端路径
                    if (ValidateTerminal(terminalPath)) {
                        // 提取终端名称
                        wchar_t terminalName[256];
                        wchar_t* fileName = wcsrchr(terminalPath, L'\\');
                        if (fileName) {
                            fileName++;
                            wcscpy(terminalName, fileName);
                        } else {
                            wcscpy(terminalName, terminalPath);
                        }

                        if (SaveTerminalConfig(terminalPath, terminalName)) {
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
                        } else {
                            MessageBoxW(hwnd, L"无法保存终端配置", L"错误", MB_OK | MB_ICONERROR);
                        }
                    } else {
                        MessageBoxW(hwnd, L"指定的终端程序路径无效", L"错误", MB_OK | MB_ICONERROR);
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

                case ID_BROWSE_BUTTON:
                {
                    wchar_t selectedPath[MAX_PATH] = L"";
                    if (BrowseForTerminal(hwnd, selectedPath, MAX_PATH)) {
                        // 验证选择的终端
                        if (ValidateTerminal(selectedPath)) {
                            // 直接设置到文本框
                            HWND hEdit = GetDlgItem(hwnd, ID_TERMINAL_EDIT);
                            SetWindowTextW(hEdit, selectedPath);
                        } else {
                            MessageBoxW(hwnd, L"选择的文件不是有效的终端程序", L"错误", MB_OK | MB_ICONERROR);
                        }
                    }
                    break;
                }

                case ID_EXIT_BUTTON:
                {
                    // 退出应用程序
                    DestroyWindow(hwnd);
                    break;
                }

                case ID_INSTALL_BUTTON:
                {
                    // 打开AI工具安装向导
                    ShowInstallationWizard(hwnd);
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

        // 使用Windows系统图标
        wcscpy(iconValue, L"shell32.dll,-25"); // 使用应用程序图标
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

        // 使用Windows系统图标
        wcscpy(iconValue, L"shell32.dll,-25"); // 使用应用程序图标
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


// 验证终端程序
BOOL ValidateTerminal(const wchar_t* terminalPath) {
    // 检查文件是否存在
    DWORD attr = GetFileAttributesW(terminalPath);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return FALSE;
    }

    // 检查文件扩展名
    const wchar_t* extension = wcsrchr(terminalPath, L'.');
    if (!extension || _wcsicmp(extension, L".exe") != 0) {
        return FALSE;
    }

    return TRUE;
}

// 保存终端配置
BOOL SaveTerminalConfig(const wchar_t* terminalPath, const wchar_t* terminalName) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, TERMINAL_CONFIG_PATH, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"TerminalPath", 0, REG_SZ,
                        (const BYTE*)terminalPath, (wcslen(terminalPath) + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"TerminalName", 0, REG_SZ,
                        (const BYTE*)terminalName, (wcslen(terminalName) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

// 读取终端配置
BOOL LoadTerminalConfig(wchar_t* terminalPath, DWORD pathSize, wchar_t* terminalName, DWORD nameSize) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, TERMINAL_CONFIG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type, size = pathSize;
        RegQueryValueExW(hKey, L"TerminalPath", NULL, &type, (LPBYTE)terminalPath, &size);

        size = nameSize;
        RegQueryValueExW(hKey, L"TerminalName", NULL, &type, (LPBYTE)terminalName, &size);
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

// 浏览终端程序
BOOL BrowseForTerminal(HWND hwnd, wchar_t* selectedPath, DWORD pathSize) {
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"可执行文件\0*.exe\0所有文件\0*.*\0";
    ofn.lpstrFile = selectedPath;
    ofn.nMaxFile = pathSize;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    return GetOpenFileNameW(&ofn);
}
