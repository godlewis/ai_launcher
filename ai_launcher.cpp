#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <string.h>

// 按钮ID定义
#define ID_CLAUDE_BUTTON 1001
#define ID_QWEN_BUTTON 1002
#define ID_CODEX_BUTTON 1003

// 窗口尺寸常量
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 280
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 35
#define BUTTON_SPACING 15
#define START_Y_POS 30

// 常量定义
#define MAX_PATH_LENGTH 1024

// 设置为Windows子系统，避免控制台窗口
#pragma comment(linker, "/subsystem:windows")
#pragma comment(lib, "comctl32.lib")

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LaunchAITool(HWND hwnd, const wchar_t* command, const wchar_t* toolName, const wchar_t* workingDir);
void ShowErrorBox(HWND hwnd, const wchar_t* message);
BOOL ValidateWorkingDirectory(const wchar_t* path);
wchar_t* ParseCommandLine(LPSTR lpCmdLine);

// 全局变量，用于键盘快捷键处理和工作目录存储
HWND g_hwnd = NULL;
wchar_t g_workingDir[MAX_PATH_LENGTH] = L"";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化通用控件
    InitCommonControls();

    // 解析命令行参数获取工作目录
    wchar_t* cmdLineWorkingDir = ParseCommandLine(lpCmdLine);
    if (cmdLineWorkingDir != NULL) {
        // 验证工作目录
        if (ValidateWorkingDirectory(cmdLineWorkingDir)) {
            wcscpy(g_workingDir, cmdLineWorkingDir);
        } else {
            wchar_t errorMsg[MAX_PATH_LENGTH + 100];
            wcscpy(errorMsg, L"指定的目录不存在或无法访问:\n");
            wcscat(errorMsg, cmdLineWorkingDir);
            MessageBoxW(NULL, errorMsg, L"错误", MB_OK | MB_ICONERROR);
            return 1;
        }
        free(cmdLineWorkingDir);
    }

    // 注册窗口类
    static const wchar_t className[] = L"AILauncher";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // 使用系统图标，看起来更像AI应用
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"注册窗口类失败!", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 计算窗口居中位置
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - WINDOW_WIDTH) / 2;
    int windowY = (screenHeight - WINDOW_HEIGHT) / 2;

    // 创建窗口 - 使用宽字符
    g_hwnd = CreateWindowExW(
        0,
        className,
        L"AI启动器",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hwnd) {
        MessageBoxW(NULL, L"创建窗口失败!", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // 消息循环 - 使用低级键盘钩子来捕获全局按键
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        // 检查快捷键
        if (msg.message == WM_KEYDOWN || msg.message == WM_CHAR) {
            switch (msg.wParam) {
                case '1':
                    LaunchAITool(g_hwnd, L"claude --dangerously-skip-permissions", L"Claude", g_workingDir);
                    continue;
                case '2':
                    LaunchAITool(g_hwnd, L"qwen -y", L"Qwen", g_workingDir);
                    continue;
                case '3':
                    LaunchAITool(g_hwnd, L"codex.cmd", L"Codex", g_workingDir);
                    continue;
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            // 计算按钮水平居中位置
            RECT rect;
            GetClientRect(hwnd, &rect);
            int buttonX = (rect.right - BUTTON_WIDTH) / 2;

            // 创建Claude按钮
            HWND hClaudeButton = CreateWindowW(
                L"BUTTON",
                L"Claude (1)",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                buttonX, START_Y_POS, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_CLAUDE_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建Qwen按钮
            HWND hQwenButton = CreateWindowW(
                L"BUTTON",
                L"Qwen (2)",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                buttonX, START_Y_POS + BUTTON_HEIGHT + BUTTON_SPACING, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_QWEN_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 创建Codex按钮
            HWND hCodexButton = CreateWindowW(
                L"BUTTON",
                L"Codex (3)",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                buttonX, START_Y_POS + 2 * (BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd,
                (HMENU)ID_CODEX_BUTTON,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );

            // 设置默认按钮焦点
            SetFocus(hClaudeButton);
            break;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            switch (wmId) {
                case ID_CLAUDE_BUTTON:
                    LaunchAITool(hwnd, L"claude --dangerously-skip-permissions", L"Claude", g_workingDir);
                    break;

                case ID_QWEN_BUTTON:
                    LaunchAITool(hwnd, L"qwen -y", L"Qwen", g_workingDir);
                    break;

                case ID_CODEX_BUTTON:
                    LaunchAITool(hwnd, L"codex", L"Codex", g_workingDir);
                    break;
            }
            break;
        }

        case WM_CHAR:
        {
            // 处理字符消息作为备选方案
            switch (wParam) {
                case '1':
                    LaunchAITool(hwnd, L"claude --dangerously-skip-permissions", L"Claude", g_workingDir);
                    break;

                case '2':
                    LaunchAITool(hwnd, L"qwen -y", L"Qwen", g_workingDir);
                    break;

                case '3':
                    LaunchAITool(hwnd, L"codex", L"Codex", g_workingDir);
                    break;
            }
            break;
        }

        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
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

void LaunchAITool(HWND hwnd, const wchar_t* command, const wchar_t* toolName, const wchar_t* workingDir) {
    // 构建完整的cmd命令
    wchar_t fullCommand[512];
    wcscpy(fullCommand, L"/k ");
    wcscat(fullCommand, command);

    // 确定工作目录
    const wchar_t* launchDir = (workingDir[0] != L'\0') ? workingDir : NULL;

    // 尝试启动AI工具
    HINSTANCE result = ShellExecuteW(
        NULL,
        L"open",
        L"cmd.exe",
        fullCommand,
        launchDir,  // 设置工作目录
        SW_SHOWNORMAL
    );

    // 检查启动结果
    if ((int)result <= 32) {
        // 启动失败，显示错误消息
        wchar_t errorMsg[512];
        wcscpy(errorMsg, toolName);
        wcscat(errorMsg, L"应用未安装或启动失败");
        ShowErrorBox(hwnd, errorMsg);
    } else {
        // 启动成功，退出主窗口
        DestroyWindow(hwnd);
    }
}

void ShowErrorBox(HWND hwnd, const wchar_t* message) {
    MessageBoxW(hwnd, message, L"错误", MB_OK | MB_ICONERROR);
}

// 解析命令行参数，提取工作目录路径
wchar_t* ParseCommandLine(LPSTR lpCmdLine) {
    if (lpCmdLine == NULL || strlen(lpCmdLine) == 0) {
        return NULL;
    }

    // 转换ANSI命令行为宽字符
    int len = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, NULL, 0);
    if (len == 0) {
        return NULL;
    }

    wchar_t* wideCmdLine = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (wideCmdLine == NULL) {
        return NULL;
    }

    MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, wideCmdLine, len);

    // 去除前后空格和引号
    wchar_t* start = wideCmdLine;
    while (*start == L' ' || *start == L'\t') {
        start++;
    }

    wchar_t* end = start + wcslen(start) - 1;
    while (end > start && (*end == L' ' || *end == L'\t')) {
        *end = L'\0';
        end--;
    }

    // 去除引号
    if (*start == L'"' && *end == L'"') {
        start++;
        *end = L'\0';
        end--;
    }

    // 如果处理后为空，返回NULL
    if (wcslen(start) == 0) {
        free(wideCmdLine);
        return NULL;
    }

    // 分配新的内存并返回路径
    wchar_t* result = (wchar_t*)malloc((wcslen(start) + 1) * sizeof(wchar_t));
    if (result != NULL) {
        wcscpy(result, start);
    }

    free(wideCmdLine);
    return result;
}

// 验证工作目录是否存在且可访问
BOOL ValidateWorkingDirectory(const wchar_t* path) {
    if (path == NULL || wcslen(path) == 0) {
        return FALSE;
    }

    // 尝试设置当前目录来验证路径
    DWORD oldAttr = SetCurrentDirectoryW(path);
    if (oldAttr != 0) {
        // 恢复原始目录（虽然程序即将退出，但这是一个好习惯）
        wchar_t currentDir[MAX_PATH_LENGTH];
        GetCurrentDirectoryW(MAX_PATH_LENGTH, currentDir);
        SetCurrentDirectoryW(currentDir);
        return TRUE;
    }

    // 作为备用检查，尝试获取文件属性
    DWORD attributes = GetFileAttributesW(path);
    return (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}