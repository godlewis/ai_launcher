#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <locale.h>

// Windows控制台颜色常量（如果未定义）
#ifndef FOREGROUND_WHITE
#define FOREGROUND_WHITE 0x07
#endif

// Windows控制台常量定义
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// Windows API版本检查
#ifndef WINVER
#define WINVER 0x0600  // Windows Vista
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  // Windows Vista
#endif



// 如果使用MinGW，使用_getch()而不是getwchar()
#ifndef _getwch
#define _getwch() (wchar_t)_getch()
#endif

// 按钮ID定义 - 保留用于兼容性
#define ID_CLAUDE_BUTTON 1001
#define ID_QWEN_BUTTON 1002
#define ID_CODEX_BUTTON 1003
#define ID_OPENCODE_BUTTON 1004
#define ID_GEMINI_BUTTON 1005
#define ID_CRUSH_BUTTON 1006
#define ID_IFLOW_BUTTON 1008
#define ID_NEOVATE_BUTTON 1009

// 常量定义
#define MAX_PATH_LENGTH 1024
#define MAX_TOOLS 8

// Windows控制台颜色常量（如果未定义）
#ifndef FOREGROUND_CYAN
#define FOREGROUND_CYAN 0x03
#endif

#ifndef FOREGROUND_YELLOW
#define FOREGROUND_YELLOW 0x0E
#endif

// 控制台颜色常量
#define COLOR_TITLE       (FOREGROUND_INTENSITY | FOREGROUND_CYAN)
#define COLOR_BORDER      (FOREGROUND_INTENSITY | FOREGROUND_WHITE)
#define COLOR_OPTION      (FOREGROUND_GREEN)
#define COLOR_EXIT        (FOREGROUND_RED)
#define COLOR_PROMPT      (FOREGROUND_INTENSITY | FOREGROUND_YELLOW)
#define COLOR_ERROR       (FOREGROUND_INTENSITY | FOREGROUND_RED)
#define COLOR_LAUNCH      (FOREGROUND_INTENSITY | FOREGROUND_GREEN)  // 显眼的亮绿色
#define COLOR_RESET       (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

// 控制台菜单尺寸常量
#define MENU_WIDTH 60

// 设置为控制台子系统，支持控制台交互
#pragma comment(linker, "/subsystem:console")

// 工具信息结构体
struct ToolInfo {
    wchar_t* name;
    wchar_t* emoji;
    wchar_t* description;
    wchar_t* command;
    int shortcutKey;
    BOOL isAvailable;
    int buttonId;
};

// 终端配置注册表路径
const wchar_t* TERMINAL_CONFIG_PATH = L"Software\\AILauncher";

// 终端参数结构
struct TerminalParams {
    wchar_t exePath[MAX_PATH];
    wchar_t args[5][256];
    int argCount;
};

// 控制台相关函数声明
void SetConsoleColor(WORD color);
void ResetConsoleColor();
void ClearScreen();
void DisplayMenu();
wchar_t HandleUserInput();
void LaunchAIToolInConsole(const wchar_t* command, const wchar_t* toolName);
void ShowErrorMessage(const wchar_t* message);
void ShowGoodbyeMessage();
void ShowNoToolsMessage();

// 计算字符串显示宽度（中文字符按2个宽度计算）
int CalculateDisplayWidth(const wchar_t* str);

// 保留的核心函数声明
BOOL ValidateWorkingDirectory(const wchar_t* path);
wchar_t* ParseCommandLine(LPSTR lpCmdLine);
BOOL IsToolAvailable(const wchar_t* command);
void InitializeToolDetection();

// 终端配置相关函数
BOOL ValidateTerminal(const wchar_t* terminalPath);
BOOL LoadTerminalConfig(wchar_t* terminalPath, DWORD pathSize, wchar_t* terminalName, DWORD nameSize);
void LaunchWithConfiguredTerminal(const wchar_t* command, const wchar_t* workingDir);

// 全局变量，用于工作目录存储
wchar_t g_workingDir[MAX_PATH_LENGTH] = L"";

// 工具信息数组 - 使用十六进制快捷键
ToolInfo g_tools[MAX_TOOLS] = {
    {L"Claude", L"[AI]", L"AI对话助手", L"claude --dangerously-skip-permissions", L'1', FALSE, ID_CLAUDE_BUTTON},
    {L"Qwen", L"[QW]", L"通义千问", L"qwen -y", L'2', FALSE, ID_QWEN_BUTTON},
    {L"Codex", L"[CD]", L"OpenAI编程助手", L"codex.cmd", L'3', FALSE, ID_CODEX_BUTTON},
    {L"OpenCode", L"[OC]", L"开源编程助手", L"opencode", L'4', FALSE, ID_OPENCODE_BUTTON},
    {L"Gemini", L"[GM]", L"Google AI编程助手", L"gemini --yolo", L'5', FALSE, ID_GEMINI_BUTTON},
    {L"Crush", L"[CR]", L"开源编程助手", L"crush", L'6', FALSE, ID_CRUSH_BUTTON},
    {L"iflow", L"[IF]", L"心流编程助手", L"iflow", L'7', FALSE, ID_IFLOW_BUTTON},
    {L"neovate", L"[NV]", L"蚂蚁金服开源编程助手", L"neovate --approval-mode yolo", L'8', FALSE, ID_NEOVATE_BUTTON}
};

// 全局可用工具数量
int g_availableToolCount = 0;

// 初始化控制台编码
void InitializeConsole() {
    // 设置locale为中文环境
    setlocale(LC_ALL, "");

    // 首先设置控制台标题
    SetConsoleTitleW(L"AI启动器 v1.0");

    // 使用系统默认编码
    SetConsoleOutputCP(GetACP());
    SetConsoleCP(GetACP());

    // 启用虚拟终端序列（ANSI转义序列支持）
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hConsole, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole, dwMode);
        }
    }

    printf("\n");
}


// 控制台功能实现
void SetConsoleColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void ResetConsoleColor() {
    SetConsoleColor(COLOR_RESET);
}

void ClearScreen() {
    system("cls");
}

void DisplayMenu() {
    ClearScreen();

    // 简化的边框，使用纯ASCII字符
    SetConsoleColor(COLOR_BORDER);
    wprintf(L"+");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        wprintf(L"-");
    }
    wprintf(L"+\n");

    // 标题行
    wprintf(L"|");
    SetConsoleColor(COLOR_TITLE);
    wprintf(L"     *** AI Tools Launcher v1.0 ***        ");
    SetConsoleColor(COLOR_BORDER);
    wprintf(L"|\n");

    // 分割线
    wprintf(L"+");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        wprintf(L"-");
    }
    wprintf(L"+\n");

    // 提示行
    wprintf(L"|");
    ResetConsoleColor();
    wprintf(L"  Press key to launch (no enter needed):       ");
    SetConsoleColor(COLOR_BORDER);
    wprintf(L"|\n");

    wprintf(L"|");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        wprintf(L" ");
    }
    wprintf(L"|\n");

    // 工具选项 - 显示固定的快捷键
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (g_tools[i].isAvailable) {
            wprintf(L"|");
            SetConsoleColor(COLOR_OPTION);

            // 格式: [1] [AI] Claude - AI对话助手
            wprintf(L"  [%lc] %ls %ls - %ls", g_tools[i].shortcutKey, g_tools[i].emoji, g_tools[i].name, g_tools[i].description);

            // 使用精确的显示宽度计算
            // 格式: "  [1] [AI] Claude - AI对话助手"
            int displayWidth = 2;  // 前导空格
            displayWidth += 1;  // [
            displayWidth += 1;  // 数字
            displayWidth += 1;  // ]
            displayWidth += 1;  // 空格
            displayWidth += CalculateDisplayWidth(g_tools[i].emoji);  // emoji显示宽度
            displayWidth += 1;  // 空格
            displayWidth += CalculateDisplayWidth(g_tools[i].name);   // 名称显示宽度
            displayWidth += 3;  // " - "
            displayWidth += CalculateDisplayWidth(g_tools[i].description); // 描述显示宽度

            int spacesNeeded = MENU_WIDTH - 4 - displayWidth;
            if (spacesNeeded < 1) spacesNeeded = 1;

            for (int j = 0; j < spacesNeeded; j++) {
                wprintf(L" ");
            }

            SetConsoleColor(COLOR_BORDER);
            wprintf(L"|\n");
        }
    }

    // 添加空行
    wprintf(L"|");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        wprintf(L" ");
    }
    wprintf(L"|\n");

    // 退出选项
    wprintf(L"|");
    SetConsoleColor(COLOR_EXIT);
    wprintf(L"  [0/q/Q] Exit Program                       ");
    SetConsoleColor(COLOR_BORDER);
    wprintf(L"|\n");

    // 底部边框
    wprintf(L"+");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        wprintf(L"-");
    }
    wprintf(L"+\n");

    ResetConsoleColor();
}

wchar_t HandleUserInput() {
    // 直接等待用户按键，无需回车
    return _getwch();
}

void LaunchAIToolInConsole(const wchar_t* command, const wchar_t* toolName) {
    wprintf(L"\n");
    SetConsoleColor(COLOR_TITLE);
    wprintf(L"正在启动 %ls...\n", toolName);
    ResetConsoleColor();

    Sleep(1000);

    // 使用配置的终端启动工具
    LaunchWithConfiguredTerminal(command, g_workingDir[0] != L'\0' ? g_workingDir : NULL);
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

// 使用配置的终端启动AI工具
void LaunchWithConfiguredTerminal(const wchar_t* command, const wchar_t* workingDir) {
    // 尝试加载用户配置的终端
    wchar_t terminalPath[MAX_PATH];
    wchar_t terminalName[256];

    if (LoadTerminalConfig(terminalPath, MAX_PATH, terminalName, 256) && ValidateTerminal(terminalPath)) {
        // 使用用户配置的终端
        wchar_t fullCommand[2048] = L"";
        wchar_t workingDirCommand[1024] = L"";

        // 确定工作目录
        const wchar_t* launchDir = (workingDir && workingDir[0] != L'\0') ? workingDir : NULL;

        // 根据不同的终端类型构建命令
        if (wcsstr(terminalPath, L"wt.exe") || wcsstr(terminalPath, L"WindowsTerminal.exe")) {
            // Windows Terminal: 使用cmd.exe作为默认配置文件
            if (launchDir) {
                wsprintfW(workingDirCommand, L"cmd /k \"cd /d \"%s\" && %s\"", workingDir, command);
            } else {
                wsprintfW(workingDirCommand, L"cmd /k \"%s\"", command);
            }
        } else if (wcsstr(terminalPath, L"powershell.exe")) {
            // PowerShell
            wcscpy(fullCommand, L"-Command");
            if (launchDir) {
                wsprintfW(workingDirCommand, L"cd \"%s\"; %s", workingDir, command);
            } else {
                wcscpy(workingDirCommand, command);
            }
            wsprintfW(fullCommand, L"%s \"%s\"", fullCommand, workingDirCommand);
        } else {
            // 默认终端 (cmd.exe)
            wcscpy(fullCommand, L"/k");
            if (launchDir) {
                wsprintfW(workingDirCommand, L"cd /d \"%s\" && %s", workingDir, command);
            } else {
                wcscpy(workingDirCommand, command);
            }
            wsprintfW(fullCommand, L"%s \"%s\"", fullCommand, workingDirCommand);
        }

        // 为终端路径添加引号
        wchar_t quotedPath[MAX_PATH * 2];
        wsprintfW(quotedPath, L"\"%s\"", terminalPath);

        // 启动终端
        HINSTANCE result = ShellExecuteW(NULL, L"open", quotedPath,
                                       (wcsstr(terminalPath, L"wt.exe") || wcsstr(terminalPath, L"WindowsTerminal.exe")) ?
                                       workingDirCommand : fullCommand,
                                       launchDir, SW_SHOWNORMAL);

        if ((int)result <= 32) {
            // 启动失败，回退到默认终端
            ShowErrorMessage(L"无法启动配置的终端程序，将使用当前控制台");
            Sleep(1500);
            _wsystem(command);
        }
    } else {
        // 没有配置终端或配置无效，使用当前控制台
        wprintf(L"\n");
        SetConsoleColor(COLOR_PROMPT);
        wprintf(L"=== 使用当前控制台 ===\n\n");
        ResetConsoleColor();
        _wsystem(command);
    }
}

void ShowErrorMessage(const wchar_t* message) {
    SetConsoleColor(COLOR_ERROR);
    wprintf(L"\n❌ 错误: %ls\n", message);
    ResetConsoleColor();
}

void ShowGoodbyeMessage() {
    ClearScreen();
    SetConsoleColor(COLOR_TITLE);
    wprintf(L"\n感谢使用AI启动器！👋\n\n");
    ResetConsoleColor();
    Sleep(1000);
}

void ShowNoToolsMessage() {
    ClearScreen();

    // 绘制边框
    SetConsoleColor(COLOR_BORDER);
    printf("+");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        printf("-");
    }
    printf("+\n");

    printf("|");
    SetConsoleColor(COLOR_TITLE);
    printf("       !!! 未检测到AI工具              ");
    SetConsoleColor(COLOR_BORDER);
    printf("|\n");

    printf("+");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        printf("-");
    }
    printf("+\n");

    printf("|");
    ResetConsoleColor();
    printf("  请安装以下AI工具之一:                   ");
    SetConsoleColor(COLOR_BORDER);
    printf("|\n");

    printf("|");
    SetConsoleColor(COLOR_OPTION);
    printf("  Claude CLI  - claude                     ");
    SetConsoleColor(COLOR_BORDER);
    printf("|\n");

    printf("|");
    SetConsoleColor(COLOR_OPTION);
    printf("  Qwen CLI    - qwen                       ");
    SetConsoleColor(COLOR_BORDER);
    printf("|\n");

    printf("|");
    SetConsoleColor(COLOR_OPTION);
    printf("  Codex CLI   - codex                      ");
    SetConsoleColor(COLOR_BORDER);
    printf("|\n");

    printf("|");
    ResetConsoleColor();
    printf("                                        |\n");

    printf("|");
    SetConsoleColor(COLOR_PROMPT);
    printf("  安装完成后请重新运行程序                  ");
    SetConsoleColor(COLOR_BORDER);
    printf("|\n");

    printf("+");
    for (int i = 0; i < MENU_WIDTH - 2; i++) {
        printf("-");
    }
    printf("+\n");

    ResetConsoleColor();

    printf("\n按任意键退出...");
    _getwch();
}

int main(int argc, char* argv[]) {
    // 初始化控制台编码
    InitializeConsole();

    // 解析命令行参数获取工作目录
    // 简化版本：如果没有参数，就不设置工作目录
    if (argc > 1) {
        // 将argv[1]转换为宽字符
        int len = MultiByteToWideChar(CP_ACP, 0, argv[1], -1, NULL, 0);
        if (len > 0) {
            wchar_t* workingDir = (wchar_t*)malloc(len * sizeof(wchar_t));
            if (workingDir != NULL) {
                MultiByteToWideChar(CP_ACP, 0, argv[1], -1, workingDir, len);

                // 验证工作目录
                if (ValidateWorkingDirectory(workingDir)) {
                    wcscpy(g_workingDir, workingDir);
                } else {
                    ShowErrorMessage(L"指定的目录不存在或无法访问");
                    free(workingDir);
                    return 1;
                }
                free(workingDir);
            }
        }
    }

    // 初始化AI工具检测
    InitializeToolDetection();

    // 检查是否为单工具自动启动场景
    if (g_availableToolCount == 1) {
        // 找到唯一的可用工具
        ToolInfo* singleTool = NULL;
        for (int i = 0; i < MAX_TOOLS; i++) {
            if (g_tools[i].isAvailable) {
                singleTool = &g_tools[i];
                break;
            }
        }

        if (singleTool != NULL) {
            wprintf(L"✓ 检测到唯一可用工具：[1] %ls %ls\n", singleTool->emoji, singleTool->name);
            SetConsoleColor(COLOR_LAUNCH);
            wprintf(L"🚀 正在自动启动 %ls...\n\n", singleTool->name);
            ResetConsoleColor();
            Sleep(1000); // 短暂延迟以显示消息

            // 使用ShellExecute启动工具，不等待其完成
            SHELLEXECUTEINFOW sei = {0};
            sei.cbSize = sizeof(SHELLEXECUTEINFOW);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.hwnd = NULL;
            sei.lpVerb = L"open";
            sei.lpFile = L"cmd.exe";

            // 构建命令行参数，在新窗口中执行工具命令
            wchar_t command[1024];
            wcscpy(command, L"/k \"");
            if (g_workingDir[0] != L'\0') {
                wcscat(command, L"cd /d \"");
                wcscat(command, g_workingDir);
                wcscat(command, L"\" && ");
            }
            wcscat(command, singleTool->command);
            wcscat(command, L"\"");

            sei.lpParameters = command;
            sei.nShow = SW_SHOWNORMAL;

            // 如果有工作目录，设置工作目录
            if (g_workingDir[0] != L'\0') {
                sei.lpDirectory = g_workingDir;
            }

            // 启动工具
            ShellExecuteExW(&sei);

            wprintf(L"✅ AI工具已在新窗口中启动\n");
            wprintf(L"ai_launcher 即将退出...\n");
            Sleep(1000);
            return 0;
        }
    }

    // 如果没有可用工具，显示安装指导
    if (g_availableToolCount == 0) {
        ShowNoToolsMessage();
        return 1;
    }

    // 主菜单循环
    while (true) {
        DisplayMenu();

        // 立即等待按键输入，无需回车
        wprintf(L"\n请按键选择: ");
        fflush(stdout); // 确保提示符立即显示

        wchar_t key = _getwch(); // 使用_getwch()获取单个按键，无需回车

        // 检查退出键
        if (key == L'0' || key == L'q' || key == L'Q' || key == 27) { // 27是ESC键
            ShowGoodbyeMessage();
            break;
        }

        // 查找匹配的工具
        bool found = false;
        for (int i = 0; i < MAX_TOOLS; i++) {
            if (g_tools[i].isAvailable && g_tools[i].shortcutKey == key) {
                found = true;
                // 显示用户选择的序号和工具信息
                wprintf(L"\n");
                SetConsoleColor(COLOR_LAUNCH);
                wprintf(L"✓ 您选择了 [%lc] %ls %ls", g_tools[i].shortcutKey, g_tools[i].emoji, g_tools[i].name);
                ResetConsoleColor();
                wprintf(L"\n");
                SetConsoleColor(COLOR_TITLE);
                wprintf(L"🚀 正在启动 %ls...", g_tools[i].name);
                ResetConsoleColor();
                wprintf(L"\n\n");

                // 启动工具后立即退出ai_launcher
                // 使用ShellExecute来启动工具，不等待其完成
                SHELLEXECUTEINFOW sei = {0};
                sei.cbSize = sizeof(SHELLEXECUTEINFOW);
                sei.fMask = SEE_MASK_NOCLOSEPROCESS;
                sei.hwnd = NULL;
                sei.lpVerb = L"open";
                sei.lpFile = L"cmd.exe";

                // 构建命令行参数，在新窗口中执行工具命令
                wchar_t command[1024];
                wcscpy(command, L"/k \"");
                if (g_workingDir[0] != L'\0') {
                    wcscat(command, L"cd /d \"");
                    wcscat(command, g_workingDir);
                    wcscat(command, L"\" && ");
                }
                wcscat(command, g_tools[i].command);
                wcscat(command, L"\"");

                sei.lpParameters = command;
                sei.nShow = SW_SHOWNORMAL;

                // 如果有工作目录，设置工作目录
                if (g_workingDir[0] != L'\0') {
                    sei.lpDirectory = g_workingDir;
                }

                // 启动工具
                ShellExecuteExW(&sei);

                // 显示启动完成消息
                wprintf(L"\n");
                SetConsoleColor(COLOR_TITLE);
                wprintf(L"✅ AI工具已在新窗口中启动");
                ResetConsoleColor();
                wprintf(L"\n");
                SetConsoleColor(COLOR_PROMPT);
                wprintf(L"ai_launcher 即将退出...");
                ResetConsoleColor();
                wprintf(L"\n");

                Sleep(1000); // 短暂延迟让用户看到消息
                return 0; // 直接退出程序
            }
        }

        // 如果没有找到匹配的工具，显示错误提示
        if (!found) {
            SetConsoleColor(COLOR_ERROR);
            wprintf(L"\n无效选择: %lc。请按数字键选择可用工具。\n", key);
            ResetConsoleColor();
            wprintf(L"按任意键重新显示菜单...");
            _getwch();
        }
    }

    return 0;
}

// 从原文件复制的核心函数实现

// 解析命令行参数
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

// 计算字符串显示宽度（中文字符按2个宽度计算）
int CalculateDisplayWidth(const wchar_t* str) {
    if (str == NULL) return 0;

    int width = 0;
    for (int i = 0; str[i] != L'\0'; i++) {
        // 中文字符范围判断
        if ((str[i] >= 0x4E00 && str[i] <= 0x9FFF) ||  // CJK统一汉字
            (str[i] >= 0x3400 && str[i] <= 0x4DBF) ||  // CJK扩展A
            (str[i] >= 0x20000 && str[i] <= 0x2A6DF) || // CJK扩展B
            (str[i] >= 0x2A700 && str[i] <= 0x2B73F) || // CJK扩展C
            (str[i] >= 0x2B740 && str[i] <= 0x2B81F) || // CJK扩展D
            (str[i] >= 0x2B820 && str[i] <= 0x2CEAF) || // CJK扩展E
            (str[i] >= 0x2CEB0 && str[i] <= 0x2EBEF) || // CJK扩展F
            (str[i] >= 0x3000 && str[i] <= 0x303F) ||  // CJK符号和标点
            (str[i] >= 0xFF00 && str[i] <= 0xFFEF)) {   // 全角ASCII、全角标点
            width += 2;
        } else {
            width += 1;
        }
    }
    return width;
}
