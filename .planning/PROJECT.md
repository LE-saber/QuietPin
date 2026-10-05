# QuietPin

## What This Is
Windows 11 轻量窗口置顶工具，C++20/原生 Win32 单 EXE，默认无托盘、无主窗口后台运行。用户通过全局快捷键切换活动窗口的置顶状态，按需打开设置。

## Core Value
无托盘后台下，能稳定地用快捷键置顶、取消置顶，并始终能打开设置或退出。

## Requirements
### Validated
暂无已交付验证的能力。
### Active
- [ ] 全局置顶快捷键、权限和系统窗口保护。
- [ ] 设置、快捷键持久化、排除程序、中文/English、登录启动和退出。
- [ ] 非激活状态提示、可选托盘。
- [ ] 后续 Pin 按钮及多屏 DPI 兼容。
### Out of Scope
账号、网络、遥测、更新服务、驱动、注入、透明度与画中画。

## Context
完整设计：[技术文档](../docs/technical-design.zh-CN.md)。2026-10-05 用户已批准按文档初始化并继续开发，要求先交付 MVP。参考方案已检查，API 已通过 Context7 和官方原文核对。

## Constraints
- 原生 Windows 11 x64、低资源占用，无额外大型运行时。
- 默认无托盘、无任务栏或主窗口；退出与恢复入口必须可靠。
- 普通权限 asInvoker；不能操作的窗口友好失败。
- 先交付阶段 1–3 MVP；Pin 不进入此次可执行版本。

## Key Decisions
| Decision | Rationale | Outcome |
| --- | --- | --- |
| C++20 + Win32 | 最小依赖，直接管理窗口 | 用户已批准 |
| 顺序执行，自动推进，Git 跟踪，保留检查与验收 | 按批准文档执行，无多代理并行 | 已采用 |
| Pin 延后 | 先稳定核心，避免跟随/DPI 风险 | MVP 范围已明确 |
| 当前以 MinGW 构建 MVP | 本机已安装；MSVC 未发现 | 正式 MSVC 验证仍待完成 |

## Evolution
阶段完成后更新验证需求、风险和关键决策；里程碑结束后重新检查范围及发布证据。未实际验证的性能与兼容要求不能标记完成。

*Last updated: 2026-10-05 after user approval*
