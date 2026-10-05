# QuietPin Roadmap

用户批准完整技术文档；本次交付阶段 1–3 的 v0.1 MVP，阶段 4–5 留待后续完整版本。

| Phase | Goal | State |
| --- | --- | --- |
| 1 | 后台核心、默认热键、筛选、身份与权限安全 | Implemented; MVP tests passed; platform gaps recorded |
| 2 | 设置、配置、语言、排除、登录启动与退出 | Implemented; MVP tests passed; real sign-in pending |
| 3 | 状态提示与可选托盘，MVP 包与核心验证 | Implemented; MVP tests passed; tray interaction gaps recorded |
| 4 | Pin 按钮、事件跟随、多显示器和 DPI | Pending |
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

### Phase 5: 完整验收
**Goal:** 完整功能在常见 Windows 11 程序上有实测兼容与低资源证据。
**Mode:** mvp
**Requirements:** COMPAT-01, PERF-01
**Success Criteria:** 兼容矩阵、2 小时运行、MSVC 构建与依赖检查记录；不冒充未测试项已通过。
