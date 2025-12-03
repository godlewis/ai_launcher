#include <windows.h>
#include <commctrl.h>
#include <string.h>

// 按钮ID定义
#define ID_CLAUDE_BUTTON 1001
#define ID_QWEN_BUTTON 1002
#define ID_CODEX_BUTTON 1003
#define ID_OPENCODE_BUTTON 1004
#define ID_GEMINI_BUTTON 1005
#define ID_CRUSH_BUTTON 1006
#define ID_IFLOW_BUTTON 1008

// 窗口尺寸常量
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 280
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 35
#define BUTTON_SPACING 15
#define START_Y_POS 30

// 常量定义
#define MAX_PATH_LENGTH 1024
#define MAX_TOOLS 7

// 设置为Windows子系统，避免控制台窗口
#pragma comment(linker, "/subsystem:windows")
#pragma comment(lib, "comctl32.lib")

// 布局模式枚举
enum LayoutMode {
    SINGLE_TOOL_AUTO_LAUNCH,  // 单工具：自动启动
    SINGLE_COLUMN,            // 2-4个工具：单列布局
    TWO_COLUMN_GRID          // 5-6个工具：2列网格布局
};

// 工具信息结构体
struct ToolInfo {
    wchar_t* name;
    wchar_t* command;
    int shortcutKey;
    BOOL isAvailable;
    HMENU buttonId;  // 改为HMENU类型以避免类型转换警告
};

// 布局信息结构体
struct LayoutInfo {
    int availableToolCount;
    LayoutMode mode;

    // 窗口尺寸
    int windowWidth;
    int windowHeight;

    // 按钮布局参数
    int buttonWidth;
    int buttonHeight;
    int buttonStartX;
    int buttonStartY;
    int buttonSpacing;
    int columnSpacing;  // 2列布局时的列间距

    // 网格布局参数
    int columns;
    int rows;
};

// 终端配置注册表路径
const wchar_t* TERMINAL_CONFIG_PATH = L"Software\\AILauncher";

// 终端参数结构
struct TerminalParams {
    wchar_t exePath[MAX_PATH];
    wchar_t args[5][256];
    int argCount;
};

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LaunchAITool(HWND hwnd, const wchar_t* command, const wchar_t* toolName, const wchar_t* workingDir);
void ShowErrorBox(HWND hwnd, const wchar_t* message);
BOOL ValidateWorkingDirectory(const wchar_t* path);
wchar_t* ParseCommandLine(LPSTR lpCmdLine);
BOOL IsToolAvailable(const wchar_t* command);
void InitializeToolDetection();
LayoutInfo CalculateLayout(int toolCount);
LayoutMode DetermineLayoutMode(int toolCount);

// 终端配置相关函数
BOOL LoadTerminalConfig(wchar_t* terminalPath, DWORD pathSize, wchar_t* terminalName, DWORD nameSize);
TerminalParams GetTerminalParams(const wchar_t* terminalPath);
BOOL ValidateTerminal(const wchar_t* terminalPath);
void LaunchWithTerminal(const wchar_t* terminalPath, const wchar_t* command, const wchar_t* workingDir);
void LaunchWithDefaultTerminal(const wchar_t* command, const wchar_t* workingDir);

// 全局变量，用于键盘快捷键处理和工作目录存储
HWND g_hwnd = NULL;
wchar_t g_workingDir[MAX_PATH_LENGTH] = L"";

// 工具信息数组
ToolInfo g_tools[MAX_TOOLS] = {
    {L"Claude", L"claude --dangerously-skip-permissions", 0, FALSE, ID_CLAUDE_BUTTON},
    {L"Qwen", L"qwen -y", 0, FALSE, ID_QWEN_BUTTON},
    {L"Codex", L"codex.cmd", 0, FALSE, ID_CODEX_BUTTON},
    {L"OpenCode", L"opencode", 0, FALSE, ID_OPENCODE_BUTTON},
    {L"Gemini", L"gemini --yolo", 0, FALSE, ID_GEMINI_BUTTON},
    {L"Crush", L"crush", 0, FALSE, ID_CRUSH_BUTTON},
    {L"iflow", L"iflow", 0, FALSE, ID_IFLOW_BUTTON}
};

// 全局可用工具数量
int g_availableToolCount = 0;

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

    // 初始化AI工具检测
    InitializeToolDetection();

    // 计算动态布局
    LayoutInfo layout = CalculateLayout(g_availableToolCount);

    // 检查是否为单工具自动启动场景
    if (layout.mode == SINGLE_TOOL_AUTO_LAUNCH) {
        // 找到唯一的可用工具
        ToolInfo* singleTool = NULL;
        for (int i = 0; i < MAX_TOOLS; i++) {
            if (g_tools[i].isAvailable) {
                singleTool = &g_tools[i];
                break;
            }
        }

        if (singleTool != NULL) {
            // 加载终端配置
            wchar_t terminalPath[MAX_PATH];
            wchar_t terminalName[256];

            if (LoadTerminalConfig(terminalPath, MAX_PATH, terminalName, 256)) {
                // 使用配置的终端启动
                LaunchWithTerminal(terminalPath, singleTool->command, g_workingDir[0] != L'\0' ? g_workingDir : NULL);
            } else {
                // 使用默认终端启动
                LaunchWithDefaultTerminal(singleTool->command, g_workingDir[0] != L'\0' ? g_workingDir : NULL);
            }
            // LaunchWithTerminal 和 LaunchWithDefaultTerminal 函数内部已经有完整的错误处理
            // 如果启动失败，用户会看到相应的错误消息
            return 0; // 直接退出程序
        } else {
            // 异常情况：应该不会发生，但作为防御性编程
            MessageBoxW(NULL, L"内部错误：检测到单工具但无法定位工具信息。\n\n程序将正常启动显示工具选择界面。",
                       L"内部错误", MB_OK | MB_ICONWARNING);
            // 继续执行到正常的UI创建流程
        }
    }

    // 注册窗口类
    static const wchar_t className[] = L"AILauncher";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // 使用Windows内置的信息图标，更具科技感
    wc.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    wc.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"注册窗口类失败!", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 主窗口循环，支持重新创建窗口
    while (true) {
        // 计算动态布局（在重新创建时可能已更新）
        LayoutInfo layout = CalculateLayout(g_availableToolCount);

        // 计算窗口居中位置
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int windowX = (screenWidth - layout.windowWidth) / 2;
        int windowY = (screenHeight - layout.windowHeight) / 2;

        // 如果窗口不存在，重新创建
        if (g_hwnd == NULL) {
            g_hwnd = CreateWindowExW(
                0,
                className,
                L"AI启动器",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                windowX, windowY, layout.windowWidth, layout.windowHeight,
                NULL, NULL, hInstance, NULL
            );

            if (!g_hwnd) {
                MessageBoxW(NULL, L"创建窗口失败!", L"错误", MB_OK | MB_ICONERROR);
                return 1;
            }
        }

        ShowWindow(g_hwnd, nCmdShow);
        UpdateWindow(g_hwnd);

        // 消息循环
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0) && g_hwnd != NULL) {
            // 检查快捷键
            if (msg.message == WM_KEYDOWN || msg.message == WM_CHAR) {
                for (int i = 0; i < MAX_TOOLS; i++) {
                    if (msg.wParam == g_tools[i].shortcutKey && g_tools[i].isAvailable) {
                        LaunchAITool(g_hwnd, g_tools[i].command, g_tools[i].name, g_workingDir);
                        continue;
                    }
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);

            // 如果窗口被销毁（重新检测），退出内层循环
            if (g_hwnd == NULL) break;
        }

        // 如果窗口被正常关闭，退出程序
        if (g_hwnd == NULL) {
            break;
        }
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            // 计算动态布局
            LayoutInfo layout = CalculateLayout(g_availableToolCount);

            HWND firstButton = NULL;

            if (g_availableToolCount == 0) {
                // 创建提示标签
                CreateWindowW(
                    L"STATIC",
                    L"未检测到可用的AI工具\n\n请确保已安装以下工具之一：\n• Claude CLI\n• Qwen CLI\n• Codex CLI\n• OpenCode CLI\n• Gemini CLI\n• Crush CLI\n• iflow CLI\n\n点击重新检测按钮重试",
                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                    20, layout.buttonStartY, layout.windowWidth - 40, 120,
                    hwnd,
                    NULL,
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                    NULL
                );

                // 创建重新检测按钮
                int recheckButtonX = (layout.windowWidth - BUTTON_WIDTH) / 2;
                CreateWindowW(
                    L"BUTTON",
                    L"重新检测",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    recheckButtonX, layout.buttonStartY + 130, BUTTON_WIDTH, BUTTON_HEIGHT,
                    hwnd,
                    (HMENU)1007, // 重新检测按钮ID
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                    NULL
                );
            } else {
                // 为每个可用的工具创建按钮
                int toolIndex = 0;
                for (int i = 0; i < MAX_TOOLS; i++) {
                    if (g_tools[i].isAvailable) {
                        // 创建按钮文本
                        wchar_t buttonText[MAX_PATH];
                        wsprintfW(buttonText, L"%s (%c)", g_tools[i].name, (wchar_t)g_tools[i].shortcutKey);

                        // 根据布局模式计算按钮位置
                        int buttonX, buttonY;
                        if (layout.mode == SINGLE_COLUMN) {
                            // 单列布局：按钮水平居中，垂直排列
                            buttonX = layout.buttonStartX;
                            buttonY = layout.buttonStartY + toolIndex * (layout.buttonHeight + layout.buttonSpacing);
                        } else if (layout.mode == TWO_COLUMN_GRID) {
                            // 2列网格布局：按网格排列
                            int column = toolIndex % 2;
                            int row = toolIndex / 2;
                            buttonX = layout.buttonStartX + column * (layout.buttonWidth + layout.columnSpacing);
                            buttonY = layout.buttonStartY + row * (layout.buttonHeight + layout.buttonSpacing);
                        } else {
                            // 不应该到达这里，但提供默认值
                            buttonX = 50;
                            buttonY = 50 + toolIndex * 50;
                        }

                        // 创建按钮
                        HWND hButton = CreateWindowW(
                            L"BUTTON",
                            buttonText,
                            WS_TABSTOP | WS_VISIBLE | WS_CHILD | (toolIndex == 0 ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                            buttonX, buttonY, layout.buttonWidth, layout.buttonHeight,
                            hwnd,
                            g_tools[i].buttonId,
                            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                            NULL
                        );

                        // 保存第一个按钮用于设置焦点
                        if (firstButton == NULL) {
                            firstButton = hButton;
                        }

                        toolIndex++;
                    }
                }
            }

            // 设置默认按钮焦点
            if (firstButton != NULL) {
                SetFocus(firstButton);
            }
            break;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            // 处理重新检测按钮
            if (wmId == 1007) {
                InitializeToolDetection();
                // 重新创建窗口
                DestroyWindow(hwnd);
                g_hwnd = NULL;
                return 0;
            }

            // 处理AI工具按钮点击
            for (int i = 0; i < MAX_TOOLS; i++) {
                if (wmId == g_tools[i].buttonId && g_tools[i].isAvailable) {
                    LaunchAITool(hwnd, g_tools[i].command, g_tools[i].name, g_workingDir);
                    break;
                }
            }
            break;
        }

        case WM_CHAR:
        {
            // 处理字符消息作为备选方案
            for (int i = 0; i < MAX_TOOLS; i++) {
                if (wParam == g_tools[i].shortcutKey && g_tools[i].isAvailable) {
                    LaunchAITool(hwnd, g_tools[i].command, g_tools[i].name, g_workingDir);
                    break;
                }
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
            // 清除全局窗口句柄，允许重新创建窗口
            if (g_hwnd == hwnd) {
                g_hwnd = NULL;
            }
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void LaunchAITool(HWND hwnd, const wchar_t* command, const wchar_t* toolName, const wchar_t* workingDir) {
    // 读取用户配置的终端程序
    wchar_t terminalPath[MAX_PATH];
    wchar_t terminalName[256];

    if (LoadTerminalConfig(terminalPath, MAX_PATH, terminalName, 256)) {
        // 验证终端程序是否仍然可用
        if (ValidateTerminal(terminalPath)) {
            // 使用用户配置的终端
            LaunchWithTerminal(terminalPath, command, workingDir);
        } else {
            // 终端不可用，显示警告并回退到默认终端
            wchar_t warningMsg[512];
            wcscpy(warningMsg, L"配置的终端程序不可用:\n");
            wcscat(warningMsg, terminalPath);
            wcscat(warningMsg, L"\n\n将使用默认的cmd.exe启动AI工具");
            MessageBoxW(hwnd, warningMsg, L"警告", MB_OK | MB_ICONWARNING);

            LaunchWithDefaultTerminal(command, workingDir);
        }
    } else {
        // 回退到默认cmd.exe
        LaunchWithDefaultTerminal(command, workingDir);
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

// 检测AI工具是否可用
BOOL IsToolAvailable(const wchar_t* command) {
    wchar_t buffer[MAX_PATH];
    wcscpy(buffer, L"where ");
    wcscat(buffer, command);

    // 使用CreateProcess静默执行where命令
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdOutput = NULL;
    si.hStdError = NULL;

    // 创建静默进程
    BOOL success = CreateProcessW(
        NULL,                           // 应用程序名称
        buffer,                         // 命令行
        NULL,                           // 进程安全属性
        NULL,                           // 线程安全属性
        FALSE,                          // 句柄继承
        CREATE_NO_WINDOW,               // 创建标志 - 不显示窗口
        NULL,                           // 环境
        NULL,                           // 当前目录
        &si,                            // 启动信息
        &pi                             // 进程信息
    );

    if (success) {
        // 等待进程完成
        WaitForSingleObject(pi.hProcess, 5000); // 最多等待5秒

        // 获取退出代码
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        // 清理进程和线程句柄
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // 如果退出代码为0，表示找到了命令
        return (exitCode == 0);
    }

    return FALSE; // 创建进程失败，认为工具不可用
}

// 初始化AI工具检测
void InitializeToolDetection() {
    g_availableToolCount = 0;

    // 检测每个AI工具
    for (int i = 0; i < MAX_TOOLS; i++) {
        // 提取命令部分（去除参数）
        wchar_t tempCommand[MAX_PATH];
        wcscpy(tempCommand, g_tools[i].command);

        // 找到第一个空格，分离命令和参数
        wchar_t* space = wcschr(tempCommand, L' ');
        if (space) {
            *space = L'\0';
        }

        // 检测工具可用性
        g_tools[i].isAvailable = IsToolAvailable(tempCommand);

        if (g_tools[i].isAvailable) {
            // 分配快捷键
            g_tools[i].shortcutKey = '1' + g_availableToolCount;
            g_availableToolCount++;
        }
    }
}

// 确定布局模式
LayoutMode DetermineLayoutMode(int toolCount) {
    if (toolCount == 1) return SINGLE_TOOL_AUTO_LAUNCH;
    if (toolCount <= 4) return SINGLE_COLUMN;
    return TWO_COLUMN_GRID;
}

// 计算动态布局信息
LayoutInfo CalculateLayout(int toolCount) {
    LayoutInfo layout = {0};
    layout.availableToolCount = toolCount;
    layout.mode = DetermineLayoutMode(toolCount);

    switch (layout.mode) {
        case SINGLE_TOOL_AUTO_LAUNCH:
            // 不需要UI，返回最小尺寸
            layout.windowWidth = 0;
            layout.windowHeight = 0;
            layout.buttonWidth = 0;
            layout.buttonHeight = 0;
            layout.columns = 0;
            layout.rows = 0;
            layout.buttonStartX = 0;
            layout.buttonStartY = 0;
            layout.buttonSpacing = 0;
            layout.columnSpacing = 0;
            break;

        case SINGLE_COLUMN: {
            // 现有的单列布局逻辑
            layout.windowWidth = 320;
            layout.buttonWidth = 200;
            layout.columns = 1;
            layout.rows = toolCount;
            layout.buttonSpacing = BUTTON_SPACING;
            layout.columnSpacing = 0;

            // 计算窗口高度
            int buttonsHeight = toolCount * BUTTON_HEIGHT + (toolCount - 1) * layout.buttonSpacing;
            layout.windowHeight = 80 + buttonsHeight + 40; // 顶部 + 按钮区域 + 底部边距

            // 计算按钮起始Y坐标（垂直居中）
            int availableHeight = layout.windowHeight - 120; // 减去顶部和底部边距
            layout.buttonStartY = 60 + (availableHeight - buttonsHeight) / 2;
            layout.buttonStartX = (layout.windowWidth - layout.buttonWidth) / 2;
            break;
        }

        case TWO_COLUMN_GRID: {
            // 新的2列布局逻辑
            layout.windowWidth = 450;
            layout.buttonWidth = 190;
            layout.columns = 2;
            layout.columnSpacing = 30;
            layout.rows = (toolCount + 1) / 2;  // 向上取整
            layout.buttonSpacing = BUTTON_SPACING;

            // 计算窗口高度
            int buttonsHeight = layout.rows * BUTTON_HEIGHT + (layout.rows - 1) * layout.buttonSpacing;
            layout.windowHeight = 80 + buttonsHeight + 40;

            // 计算按钮起始Y坐标（垂直居中）
            int availableHeight = layout.windowHeight - 120;
            layout.buttonStartY = 60 + (availableHeight - buttonsHeight) / 2;

            // 计算按钮起始X坐标（网格居中）
            int totalGridWidth = layout.columns * layout.buttonWidth + (layout.columns - 1) * layout.columnSpacing;
            layout.buttonStartX = (layout.windowWidth - totalGridWidth) / 2;
            break;
        }
    }

    // 设置通用参数
    layout.buttonHeight = BUTTON_HEIGHT;

    return layout;
}

// 读取终端配置
BOOL LoadTerminalConfig(wchar_t* terminalPath, DWORD pathSize, wchar_t* terminalName, DWORD nameSize) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, TERMINAL_CONFIG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type, size = pathSize;
        DWORD result = RegQueryValueExW(hKey, L"TerminalPath", NULL, &type, (LPBYTE)terminalPath, &size);

        if (result == ERROR_SUCCESS && terminalPath[0] != L'\0') {
            size = nameSize;
            RegQueryValueExW(hKey, L"TerminalName", NULL, &type, (LPBYTE)terminalName, &size);
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }
    return FALSE;
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

// 获取终端启动参数
TerminalParams GetTerminalParams(const wchar_t* terminalPath) {
    TerminalParams params;
    wcscpy(params.exePath, terminalPath);
    params.argCount = 0;

    if (wcsstr(terminalPath, L"wt.exe") || wcsstr(terminalPath, L"WindowsTerminal.exe")) {
        // Windows Terminal
        wcscpy(params.args[0], L"new-tab");
        wcscpy(params.args[1], L"--");
        wcscpy(params.args[2], L"cmd.exe");
        wcscpy(params.args[3], L"/k");
        params.argCount = 4;
    } else if (wcsstr(terminalPath, L"powershell.exe")) {
        // PowerShell
        wcscpy(params.args[0], L"-Command");
        params.argCount = 1;
    } else if (wcsstr(terminalPath, L"git-bash.exe")) {
        // Git Bash
        wcscpy(params.args[0], L"-l");
        params.argCount = 1;
    } else {
        // 默认参数（cmd.exe和其他终端）
        wcscpy(params.args[0], L"/k");
        params.argCount = 1;
    }

    return params;
}

// 使用指定终端启动AI工具
void LaunchWithTerminal(const wchar_t* terminalPath, const wchar_t* command, const wchar_t* workingDir) {
    wchar_t fullCommand[2048] = L"";
    wchar_t workingDirCommand[1024] = L"";

    // 确定工作目录
    const wchar_t* launchDir = (workingDir && workingDir[0] != L'\0') ? workingDir : NULL;

    // 根据不同的终端类型构建命令
    if (wcsstr(terminalPath, L"wt.exe") || wcsstr(terminalPath, L"WindowsTerminal.exe")) {
        // Windows Terminal: 使用cmd.exe作为默认配置文件
        wcscpy(fullCommand, L"");
        if (launchDir) {
            // 直接使用cmd的完整命令
            wsprintfW(workingDirCommand, L"cmd /k \"cd /d \"%s\" && %s\"", workingDir, command);
        } else {
            wsprintfW(workingDirCommand, L"cmd /k \"%s\"", command);
        }
    } else if (wcsstr(terminalPath, L"powershell.exe")) {
        // PowerShell: powershell.exe -Command "cd DIR; AI_TOOL_COMMAND"
        wcscpy(fullCommand, L"-Command");
        if (launchDir) {
            wsprintfW(workingDirCommand, L"cd \"%s\"; %s", workingDir, command);
        } else {
            wcscpy(workingDirCommand, command);
        }
    } else if (wcsstr(terminalPath, L"git-bash.exe")) {
        // Git Bash: git-bash.exe -l -c "cd DIR && AI_TOOL_COMMAND"
        wcscpy(fullCommand, L"-l -c");
        if (launchDir) {
            // Git Bash使用Unix风格路径，需要转换Windows路径
            wchar_t unixPath[MAX_PATH];
            wcscpy(unixPath, workingDir);
            // 将反斜杠转换为正斜杠
            for (wchar_t* p = unixPath; *p; p++) {
                if (*p == L'\\') *p = L'/';
            }
            wsprintfW(workingDirCommand, L"cd '%s' && %s", unixPath, command);
        } else {
            wcscpy(workingDirCommand, command);
        }
    } else {
        // 默认终端 (cmd.exe): cmd.exe /k "cd /d DIR && AI_TOOL_COMMAND"
        wcscpy(fullCommand, L"/k");
        if (launchDir) {
            wsprintfW(workingDirCommand, L"cd /d \"%s\" && %s", workingDir, command);
        } else {
            wcscpy(workingDirCommand, command);
        }
    }

    // 为终端路径添加引号，处理包含空格的路径
    wchar_t quotedPath[MAX_PATH * 2];
    wsprintfW(quotedPath, L"\"%s\"", terminalPath);

    // 启动终端
    HINSTANCE result;
    if (wcsstr(terminalPath, L"wt.exe") || wcsstr(terminalPath, L"WindowsTerminal.exe")) {
        // Windows Terminal: 直接传递cmd命令
        result = ShellExecuteW(NULL, L"open", quotedPath, workingDirCommand, launchDir, SW_SHOWNORMAL);
    } else if (wcsstr(terminalPath, L"powershell.exe")) {
        // PowerShell: 直接将命令作为参数传递
        wchar_t finalCommand[3072];
        wsprintfW(finalCommand, L"%s \"%s\"", fullCommand, workingDirCommand);
        result = ShellExecuteW(NULL, L"open", quotedPath, finalCommand, launchDir, SW_SHOWNORMAL);
    } else if (wcsstr(terminalPath, L"git-bash.exe")) {
        // Git Bash: 直接将命令作为参数传递
        wchar_t finalCommand[3072];
        wsprintfW(finalCommand, L"%s \"%s\"", fullCommand, workingDirCommand);
        result = ShellExecuteW(NULL, L"open", quotedPath, finalCommand, launchDir, SW_SHOWNORMAL);
    } else {
        // cmd.exe
        wchar_t finalCommand[3072];
        wsprintfW(finalCommand, L"%s \"%s\"", fullCommand, workingDirCommand);
        result = ShellExecuteW(NULL, L"open", quotedPath, finalCommand, launchDir, SW_SHOWNORMAL);
    }

    // 检查启动结果
    if ((int)result <= 32) {
        // 启动失败，回退到默认终端
        wchar_t errorMsg[512];
        wcscpy(errorMsg, L"无法启动配置的终端程序:\n");
        wcscat(errorMsg, terminalPath);
        wcscat(errorMsg, L"\n\n将使用默认终端重试");
        MessageBoxW(NULL, errorMsg, L"错误", MB_OK | MB_ICONERROR);

        LaunchWithDefaultTerminal(command, workingDir);
    } else {
        // 启动成功，退出主窗口
        if (g_hwnd) {
            DestroyWindow(g_hwnd);
        }
    }
}

// 使用默认终端启动AI工具
void LaunchWithDefaultTerminal(const wchar_t* command, const wchar_t* workingDir) {
    wchar_t fullCommand[1024];

    // 构建完整的cmd命令，包含工作目录切换
    if (workingDir && workingDir[0] != L'\0') {
        wsprintfW(fullCommand, L"/k \"cd /d \"%s\" && %s\"", workingDir, command);
    } else {
        wsprintfW(fullCommand, L"/k %s", command);
    }

    // 确定工作目录
    const wchar_t* launchDir = (workingDir && workingDir[0] != L'\0') ? workingDir : NULL;

    // 启动默认cmd.exe
    HINSTANCE result = ShellExecuteW(
        NULL,
        L"open",
        L"cmd.exe",
        fullCommand,
        launchDir,
        SW_SHOWNORMAL
    );

    // 检查启动结果
    if ((int)result <= 32) {
        // 启动失败，显示错误消息
        MessageBoxW(NULL, L"无法启动终端，请检查系统设置", L"错误", MB_OK | MB_ICONERROR);
    } else {
        // 启动成功，退出主窗口
        if (g_hwnd) {
            DestroyWindow(g_hwnd);
        }
    }
}
