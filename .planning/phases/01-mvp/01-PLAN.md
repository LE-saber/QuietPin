---
phase: 01-mvp
plan: 01
type: execute
wave: 1
depends_on: []
autonomous: true
---
# MVP execution plan

## Tracer
原生可执行文件后台启动 → WM_HOTKEY → 当前合格前台 → SetWindowPos 异步切换 → 实际状态确认 → 非激活反馈。用受控 Win32 宿主验证完整路径。

## Tasks
1. 创建 CMake/manifest、配置与窗口策略模块；实现身份、排除、权限判断。验证编译与配置/策略测试。
2. 实现隐藏宿主、单实例/命令行、全局热键、设置、语言、退出、登录启动事务。验证冲突/持久化及无托盘恢复。
3. 实现非激活提示、可选托盘与退出清理。验证焦点保持、托盘目标与实例控制。
4. 执行本机集成测试，检查导入 DLL、体积、空闲 CPU，打包 README/EXE/校验值。记录未验证项。

## Must haves
- 默认无可见 UI，Ctrl+Alt+T 切换，Ctrl+Alt+Shift+T 设置，Ctrl+Alt+Shift+Q 退出。
- 无托盘仍可用第二次运行或 --settings/--exit 管理。
- 冲突/受限/排除/无响应失败不会终止进程或虚报成功。
- Pin 不显示、不启用；文档明确 MVP 尚未包含。

## Plan check
对照已批准技术文档：MVP 包含阶段 1–3；所有 Pin 要求在阶段 4；完整兼容/长时间指标在阶段 5，不阻碍交付诚实标注的 MVP。
