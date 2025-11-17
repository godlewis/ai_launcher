# AI启动器使用说明

## 简介
AI启动器是一个便捷的Windows桌面应用程序，提供多个AI工具的快速启动入口，支持自定义工作目录。

## 功能特性
- 🚀 一键启动Claude、Qwen、Codex三个AI工具
- ⌨️ 支持键盘快捷键操作（1、2、3）
- 📁 支持命令行参数指定工作目录
- 🌍 完整继承用户环境变量
- 🎨 美观的三按钮界面布局
- ❌ 启动后自动退出，节省系统资源
- ⚠️ 智能错误检测和提示

## 使用方法

### 鼠标操作
1. 启动应用程序
2. 点击对应的AI工具按钮：
   - **Claude (1)** - 启动Claude CLI（带权限跳过）
   - **Qwen (2)** - 启动Qwen CLI（自动确认）
   - **Codex (3)** - 启动Codex CLI

### 键盘快捷键
- 按 **数字键 1** - 启动Claude（在任何时候都有效）
- 按 **数字键 2** - 启动Qwen（在任何时候都有效）
- 按 **数字键 3** - 启动Codex（在任何时候都有效）
- 按 **ESC键** - 退出应用程序

### 命令行参数
AI启动器支持可选的工作目录参数：

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

#### 参数说明：
- **支持绝对路径**：如 `C:\MyProject` 或 `D:\AI\workspace`
- **支持相对路径**：如 `.\project` 或 `..\workspace`
- **支持包含空格的路径**：使用引号包围，如 `"C:\Program Files\My App"`
- **向后兼容**：无参数时使用默认行为

#### 环境变量继承：
- ✅ 自动继承当前用户的所有环境变量
- ✅ 包括 PATH、HOME、APPDATA 等系统变量
- ✅ 包括用户自定义的环境变量
- ✅ 确保 AI 工具能够访问开发工具和配置

## 技术细节

### 启动的命令
- **Claude**: `claude --dangerously-skip-permissions`
- **Qwen**: `qwen -y`
- **Codex**: `codex`

### 错误处理

#### AI工具启动失败
如果某个AI工具未安装或启动失败，会显示相应的错误提示：
- "Claude应用未安装或启动失败"
- "Qwen应用未安装或启动失败"
- "Codex应用未安装或启动失败"

#### 工作目录错误
如果指定的工作目录不存在或无法访问，会显示错误提示并退出：
- "指定的目录不存在或无法访问: [路径]"

## 使用场景

### 开发环境
```bash
# 在项目目录中启动AI工具，可以直接访问项目文件
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

## 系统要求
- Windows 7 或更高版本
- 已安装对应的AI工具（Claude、Qwen、Codex）
- 足够的权限执行命令行工具
- 对指定工作目录的读取权限

## 文件说明
- `ai_launcher.exe` - 主程序文件
- `ai_launcher.cpp` - 源代码
- `claude_launcher_chinese_backup.cpp` - 原单按钮版本备份

## 注意事项
1. 首次使用前请确保已安装对应的AI工具
2. 应用程序启动AI工具后会自动退出
3. 终端进程会独立运行，不会受到应用程序退出影响
4. 如果遇到启动失败，请检查AI工具是否正确安装和配置

## 版本信息
- 版本：2.0.0（支持工作目录参数）
- 编译时间：2024年11月
- 开发语言：C++ (Win32 API)

## 常见问题

**Q: 如何在包含空格的路径中使用？**
A: 使用双引号包围路径，如 `ai_launcher.exe "C:\Program Files\My App"`

**Q: 支持网络驱动器路径吗？**
A: 支持，如 `ai_launcher.exe "Z:\NetworkShare\Project"`

**Q: 环境变量会继承吗？**
A: 是的，AI工具会自动继承当前用户的所有环境变量

**Q: 如果没有指定工作目录会怎样？**
A: 使用默认行为，与之前版本一致

**Q: 可以使用相对路径吗？**
A: 可以，如 `ai_launcher.exe ".\project"`