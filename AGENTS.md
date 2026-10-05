# QuietPin project instructions

按 docs/technical-design.zh-CN.md 和 .planning/ROADMAP.md 开发。v0.1 MVP 已发布，用户确认测试成功。阶段 4 Pin 方案见 docs/pin-implementation-plan.zh-CN.md 和 .planning/phases/04-pin/；当前用户已授权实现 v0.2，并要求 Windows 安装版及便携 ZIP。使用原生 C++20/Win32，避免大型依赖与默认轮询；普通权限运行。

涉及库/API/CLI 文档时先用 Context7 resolve-library-id，再按概念 query-docs；官方文档补充复核。不得把仅编译通过或纯单元测试描述为所有 Windows 11 程序兼容。

每个可运行阶段提交，更新 STATE/验证记录；既有用户批准允许继续实施，不重复要求技术选型确认。未明确请求代理时当前会话顺序执行，不启动子代理。
