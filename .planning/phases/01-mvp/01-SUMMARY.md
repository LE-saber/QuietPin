# MVP Summary

## Completed
阶段 1–3 已实现为可运行 v0.1：原生后台热键、单实例、统一窗口保护、配置、语言、排除、登录启动、设置/退出、非激活提示和可选托盘。

## Verification
MinGW Release 构建成功，Core 与 Win32 集成测试 2/2 通过；真实键盘输入、冲突回滚、焦点、损坏恢复、重启持久化、退出恢复有证据。最终 EXE 1,251,840 bytes；60 秒空闲 1 线程、Private Bytes 约 1.65 MiB、采样内 CPU 时间增量 0。详细记录见 docs/verification.zh-CN.md。

## Deviations
MVP 将 UI/宿主集中在 main.cpp，纯配置和窗口策略分开。设置/退出热键固定；Pin 留到阶段 4。本机 MinGW 代替尚未安装/验证的 MSVC，没有伪称 MSVC 通过。

## Remaining
阶段 4 Pin；阶段 5 兼容、多屏、提升权限/无响应目标、登录后启动、Explorer 重启和长时间验证。MVP 可交付不等同于完整路线完成。
