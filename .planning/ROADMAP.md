# QuietPin Roadmap — v0.2

用户批准完整技术文档；阶段 1–3 的 v0.1 MVP 已交付并获用户测试确认。v0.1.0 已发布；阶段 4 v0.2 已交付，用户确认人工测试通过并批准正式发布；更广的平台验证保留阶段 5。

| Phase | Goal | State |
| --- | --- | --- |
| 1 | 后台核心、默认热键、筛选、身份与权限安全 | Implemented; MVP tests passed; platform gaps recorded |
| 2 | 设置、配置、语言、排除、登录启动与退出 | Implemented; MVP tests passed; real sign-in pending |
| 3 | 状态提示与可选托盘，MVP 包与核心验证 | Implemented; MVP tests passed; tray interaction gaps recorded |
| 4 | Pin 按钮、事件跟随、多显示器和 DPI | Released; 3/3 summaries; user acceptance confirmed |
| 5 | 完整兼容矩阵、长时间性能、MSVC/签名发布准备 | Pending |

### Phase 1: 后台核心

**Goal:** 不显示 UI 的用户可用默认热键切换合格活动窗口的实际置顶状态。
**Mode:** mvp
**Requirements:** CORE-01, CORE-02, CORE-03, CORE-04, SAFE-01, SAFE-02, SAFE-03
**Success Criteria:** 无 Shell 操作；受限目标失败不崩溃；长按不重复；目标关闭安全；普通启动无 UI。

### Phase 2: 可管理后台

**Goal:** 无托盘用户可修改配置、排除程序、登录启动并退出。
**Mode:** mvp
**Requirements:** CFG-01, CFG-02, CFG-03, CFG-04, CFG-05, CFG-06, CFG-09, CFG-10, EXCL-01, EXCL-02
**Success Criteria:** 热键冲突不丢旧配置；持久化和中文路径可靠；设置/退出始终有命令行入口。

### Phase 3: 可选反馈与 MVP

**Goal:** 用户可启停非激活提示和托盘，并获得独立可运行的 MVP 包。
**Mode:** mvp
**Requirements:** CFG-07, UX-01, TRAY-01, TRAY-02, EXIT-01
**Success Criteria:** 提示不激活；托盘冻结正确目标；Explorer 重启重新注册；完整退出。

### Phase 4: Pin

**Goal:** 用户启用后获得可点击的非激活 Pin，支持移动与多屏。
**Mode:** mvp
**Requirements:** CFG-08, PIN-01, PIN-02, PIN-03, PIN-04
**Success Criteria:** 无残留、不遮挡系统按钮；混合 DPI 与负坐标正确；关闭后无跟随计时器。

**Depends on:** Phase 3
**Plans:** 3 plans，顺序实施。完整方案见 docs/pin-implementation-plan.zh-CN.md。
**Wave 1**

- [x] 04-01-PLAN.md — 配置/前台共享订阅/非激活点击最小完整路径。

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 04-02-PLAN.md — 事件跟随/安全位置/偏移/多屏 DPI 与会话失效。

**Wave 3** *(blocked on Wave 2 completion)*

- [x] 04-03-PLAN.md — 辅助功能/竞争/资源/应用矩阵与 v0.2 候选包。

**Cross-cutting constraints:**

- D-01: 默认关闭、无托盘后台能力保持；关闭后释放 Pin 窗口与专属资源。
- D-10: 原生 Win32 单消息循环，无注入、跨进程父子窗口或大型依赖。

### Phase 5: 完整验收

**Goal:** 完整功能在常见 Windows 11 程序上有实测兼容与低资源证据。
**Mode:** mvp
**Requirements:** COMPAT-01, PERF-01
**Success Criteria:** 兼容矩阵、2 小时运行、MSVC 构建与依赖检查记录；不冒充未测试项已通过。

## Progress

此表供 GSD 工具读取。阶段 1–3 共用已完成的 01-mvp/01-PLAN；MVP 已交付，但三个阶段保留平台手工缺口，故仍为 In progress。阶段 4 三份实施计划已有 SUMMARY，用户确认发布验收通过；阶段 4 交付完成，更广的平台矩阵由阶段 5 跟踪。

| Phase | Plans Complete | Status | Completed |
| --- | --- | --- | --- |
| 1. 后台核心 | 1/1 | In progress | MVP verified 2026-10-05; platform gaps pending |
| 2. 可管理后台 | 0/0 | In progress | Shared MVP plan; real sign-in pending |
| 3. 可选反馈与 MVP | 0/0 | In progress | Shared MVP plan; manual tray gaps pending |
| 4. Pin | 3/3 | Complete | User acceptance and stable release 2026-10-05 |
| 5. 完整验收 | 0/0 | Not started | — |

