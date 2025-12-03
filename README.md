# AI启动器 (AI Launcher)

一个强大的Windows桌面AI工具启动器，支持多种AI工具快速启动、可配置终端选择和工作目录右键菜单集成。

## 🖼️ 界面预览

程序使用Windows系统图标，界面简洁现代：
- 主程序使用系统应用程序图标
- 右键菜单使用终端相关图标 (shell32.dll,-25)

## 🚀 功能特性

### 核心功能
- 🤖 **多AI工具支持**：智能检测并启动Claude、Qwen、Codex、OpenCode、Gemini、Crush、iflow等AI工具
- ⌨️ **键盘快捷键**：动态分配数字键1、2、3、4、5、6、7快速启动
- 📁 **工作目录支持**：支持命令行参数指定工作目录
- 🌍 **环境变量继承**：完整继承用户环境变量和PATH配置
- 🖱️ **右键菜单集成**：支持文件夹和文件夹背景右键菜单
- ⚙️ **终端配置**：可配置不同终端程序(cmd.exe, PowerShell, Windows Terminal, Git Bash等)
- 🎨 **系统图标**：使用Windows系统图标，确保兼容性和一致性
- ❌ **自动清理**：启动AI工具后自动退出
- ⚠️ **错误检测**：智能错误检测和友好提示

### 终端配置功能
- 🔧 **多终端支持**：支持cmd.exe、PowerShell、Windows Terminal、Git Bash等
- 📁 **绿色软件支持**：可浏览选择任意位置的便携版终端
- 🖥️ **智能适配**：自动适配不同终端的参数格式和工作目录
- 💾 **配置持久化**：用户配置保存到注册表，重启后保持设置
- 🔄 **动态启动**：根据选择的终端和类型动态调整启动参数

### 右键菜单功能
- 📂 **双位置支持**：文件夹右键 + 文件夹背景右键
- 🏠 **智能路径传递**：自动传递当前目录路径给AI工具
- 🔒 **安全卸载**：完全可卸载，无残留
- 🛡️ **权限管理**：自动请求管理员权限
- 📱 **图标集成**：右键菜单显示Windows系统终端图标
- 📱 **绿色部署**：无外部依赖，单文件便携部署

## 📦 文件说明

| 文件 | 说明 |
|------|------|
| `ai_launcher.exe` | 主程序 - AI工具启动器 (44KB) |
| `registry_manager.exe` | 右键菜单管理器 (43KB) |
| `ai_launcher.cpp` | 主程序源代码 |
| `registry_manager.cpp` | 注册表管理器源代码 |

## 🎯 使用方法

### 1. 配置终端程序

#### 首次配置
1. 运行 `registry_manager.exe`
2. 在"终端程序选择"区域填写终端路径：
   - 点击"浏览..."按钮选择终端程序
   - 或直接在文本框中输入终端路径（如：`C:\Windows\System32\cmd.exe`）
3. 点击"注册右键菜单"完成配置

#### 支持的终端类型
- **cmd.exe** - Windows命令提示符
- **powershell.exe** - Windows PowerShell
- **wt.exe** - Windows Terminal
- **git-bash.exe** - Git Bash
- **其他便携终端** - 支持选择任意位置的便携版终端

### 2. 直接启动AI工具

#### 鼠标操作
1. 双击运行 `ai_launcher.exe`
2. 点击对应的AI工具按钮：
   - **Claude (1)** - 启动Claude CLI（带权限跳过）
   - **Qwen (2)** - 启动Qwen CLI（自动确认）
   - **Codex (3)** - 启动Codex CLI
   - **OpenCode (4)** - 启动OpenCode CLI
   - **Gemini (5)** - 启动Gemini CLI（带--yolo参数）
   - **Crush (6)** - 启动Crush CLI
   - **iflow (7)** - 启动iflow CLI

#### 键盘快捷键
- **按 1** - 启动Claude
- **按 2** - 启动Qwen
- **按 3** - 启动Codex
- **按 4** - 启动OpenCode
- **按 5** - 启动Gemini
- **按 6** - 启动Crush
- **按 7** - 启动iflow
- **按 ESC** - 退出程序

### 3. 命令行参数（工作目录）

```bash
# 在默认目录启动
ai_launcher.exe

# 在指定目录启动
ai_launcher.exe "C:\MyProject"
ai_launcher.exe "D:\AI\workspace"
ai_launcher.exe "C:\Program Files\My App"

# 使用相对路径
ai_launcher.exe ".\project"
ai_launcher.exe "..\workspace"
```

#### 参数说明
- ✅ **支持绝对路径**：`C:\MyProject`
- ✅ **支持相对路径**：`.\project`
- ✅ **支持空格路径**：使用引号包围
- ✅ **向后兼容**：无参数时使用默认行为
- ✅ **环境变量继承**：自动继承PATH、HOME等所有环境变量

### 4. 右键菜单集成

#### 安装右键菜单
1. **运行注册器**：
   ```bash
   # 方法一：右键 registry_manager.exe → "以管理员身份运行"
   # 方法二：双击运行，程序自动请求管理员权限
   ```

2. **注册菜单**：点击"注册右键菜单"按钮
3. **等待完成**：等待注册成功提示

#### 使用右键菜单
1. 在任意文件夹上右键点击
2. 选择 **"用AI工具打开"**
3. AI启动器将在该目录下启动
4. 选择需要的AI工具

#### 支持的右键位置
- 📁 **文件夹右键**：在文件夹上右键
- 🏠 **文件夹背景右键**：在文件夹空白处右键

#### 卸载右键菜单
1. 以管理员身份运行 `registry_manager.exe`
2. 点击"卸载右键菜单"按钮
3. 等待卸载完成

## 🛠️ 技术细节

### AI工具启动命令
- **Claude**: `claude --dangerously-skip-permissions`
- **Qwen**: `qwen -y`
- **Codex**: `codex.cmd`
- **OpenCode**: `opencode`
- **Gemini**: `gemini --yolo`
- **Crush**: `crush`
- **iflow**: `iflow`

### 终端适配说明
程序会自动根据选择的终端类型调整启动参数：

- **cmd.exe**: 使用 `/k` 参数保持窗口打开，直接执行AI工具命令
- **PowerShell**: 使用 `-NoExit` 参数保持窗口打开，执行AI工具命令
- **Windows Terminal**: 通过cmd.exe包装，确保正确的工作目录设置
- **Git Bash**: 使用 `--cd=` 参数设置工作目录，执行AI工具命令
- **其他终端**: 默认使用cmd.exe兼容模式

### 右键菜单注册表位置
- `HKEY_CLASSES_ROOT\Directory\shell\AITools` - 文件夹右键
- `HKEY_CLASSES_ROOT\Directory\Background\shell\AITools` - 文件夹背景右键

### 启动命令格式
```
"[ai_launcher.exe完整路径]" "[目录路径]"
```

## 📋 使用场景

### 开发环境
```bash
# 在项目目录中启动AI工具，直接访问项目文件
ai_launcher.exe "C:\MyPythonProject"
ai_launcher.exe "D:\WebDev\MySite"
```

### 机器学习项目
```bash
# 在数据科学工作区中启动，访问数据和脚本
ai_launcher.exe "E:\ML\datasets"
```

### 文档处理
```bash
# 在文档目录中启动AI写作助手
ai_launcher.exe "C:\Users\Name\Documents\Articles"
```

### 右键快速启动
- 在任意项目文件夹右键 → "用AI工具打开"
- 在代码库文件夹背景右键 → "用AI工具打开"
- 在文档目录右键 → "用AI工具打开"

## ⚙️ 系统要求

- **操作系统**：Windows 7 或更高版本
- **AI工具**：已安装对应的Claude、Qwen、Codex、OpenCode、Gemini、Crush、iflow CLI工具
- **权限**：注册右键菜单需要管理员权限
- **磁盘空间**：约5MB

## 🔧 故障排除

### 常见问题

#### Q: 右键菜单没有显示怎么办？
A:
1. 确保注册操作成功完成
2. 重启Windows资源管理器（Ctrl+Shift+Esc → 结束explorer.exe → 新建任务explorer.exe）
3. 检查杀毒软件是否阻止了注册表修改

#### Q: 为什么需要管理员权限？
A: 注册表操作需要写入系统注册表，这需要管理员权限。使用右键菜单不需要特殊权限。

#### Q: 移动了ai_launcher.exe文件位置怎么办？
A:
1. 卸载当前的右键菜单
2. 将ai_launcher.exe移动到新位置
3. 重新注册右键菜单

#### Q: 如何知道右键菜单是否已注册？
A: 运行registry_manager.exe，界面上会显示当前注册状态。

#### Q: 支持哪些Windows版本？
A: 支持Windows 7及更高版本，主要针对Windows 10/11优化。

#### Q: 环境变量会继承吗？
A: 是的，AI工具会自动继承当前用户的所有环境变量，包括PATH、HOME、APPDATA等。

#### Q: 可以使用相对路径和网络驱动器吗？
A: 支持。相对路径如 `.\project`，网络驱动器路径如 `Z:\NetworkShare\Project`。

#### Q: 如何切换终端类型？
A: 运行 `registry_manager.exe`，在终端程序选择区域输入新的终端路径，然后重新注册右键菜单即可。

#### Q: 支持便携版终端吗？
A: 支持。点击"浏览..."按钮可以选择任意位置的便携版终端程序。

#### Q: Windows Terminal启动失败怎么办？
A: 确保Windows Terminal已正确安装，或尝试使用cmd.exe作为终端程序。

### 错误处理

#### AI工具启动失败
如果AI工具未安装或启动失败，会显示：
- "Claude应用未安装或启动失败"
- "Qwen应用未安装或启动失败"
- "Codex应用未安装或启动失败"
- "OpenCode应用未安装或启动失败"
- "Gemini应用未安装或启动失败"
- "Crush应用未安装或启动失败"
- "iflow应用未安装或启动失败"

#### 工作目录错误
如果指定的工作目录不存在，会显示：
- "指定的目录不存在或无法访问: [路径]"

### 手动清理

#### 注册失败时
1. 确保以管理员身份运行
2. 检查ai_launcher.exe是否在同一目录
3. 暂时关闭杀毒软件重试
4. 检查系统注册表是否被保护

#### 手动卸载注册表项
```
HKEY_CLASSES_ROOT\Directory\shell\AITools
HKEY_CLASSES_ROOT\Directory\Background\shell\AITools
```

## 🔒 安全说明

- ✅ 仅修改标准Windows Shell集成注册表项
- ✅ 不会修改系统关键设置
- ✅ 提供完整的卸载功能
- ✅ 遵循Windows Shell集成最佳实践
- ✅ 自包含部署，无外部依赖

## 📝 版本信息

- **版本**：1.0.8
- **编译时间**：2024年12月3日
- **开发语言**：C++ (Win32 API)
- **目标系统**：Windows 10/11（兼容Windows 7+）
- **新增特性**：支持iflow新AI工具，扩展工具支持至7个，优化7工具布局为2列4行显示

## 🚀 快速开始

1. **下载程序**：获取 `ai_launcher.exe` 和 `registry_manager.exe`
2. **安装右键菜单**：以管理员身份运行 `registry_manager.exe` 并点击注册
3. **开始使用**：
   - 双击 `ai_launcher.exe` 直接启动
   - 或在任何文件夹右键选择"用AI工具打开"
4. **享受AI工具**：使用数字键1、2、3、4、5、6、7快速启动对应AI工具

---

**Made with ❤️ for AI Developers**