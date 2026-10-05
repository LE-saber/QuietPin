# Phase 4: Pin — Context

**Gathered:** 2026-10-05
**Status:** Ready for planning
**Source:** PRD Express Path：已批准 docs/technical-design.zh-CN.md 第 10 节，以及本次用户“先上传现有版本，然后给出下一阶段计划”的要求。

<domain>
## Phase Boundary
为原生 Win32 MVP 增加用户主动开启的活动窗口 Pin 按钮，覆盖 CFG-08、PIN-01–04。本轮仅规划；阶段执行后预期版本 v0.2.0。v0.1.0 用户已确认测试成功并发布；未覆盖的平台验证仍保留。
</domain>

<decisions>
## Implementation Decisions

### 默认与交互
- **D-01:** Pin 默认关闭；无托盘、无主窗口、普通权限和原有热键行为保持，关闭 Pin 后释放 overlay、专属 hook 与计时器。
- **D-02:** 独立非激活 popup，点击必须送达；按下/释放核对当前前台、身份与绑定代次；调用既有置顶和异步确认路径，不操作托盘缓存目标。
- **D-03:** GDI 绘制清晰的两种真实状态、悬停/按下/忙碌状态，提供中英文本提示和可访问名称；失败不虚报成功。

### 跟随与安全
- **D-04:** 事件驱动；回调只投递合并刷新，几何事件按目标过滤，移动期间才允许短期补偿；托盘与 Pin 的前台订阅生命周期独立。
- **D-05:** 目标切换、排除/权限失败、销毁/最小化/隐藏、cloaking、设置激活、会话锁定和退出均取消手势并隐藏；恢复时重新读取前台，不能复活旧目标。
- **D-06:** PerMonitorV2，物理屏幕坐标、可见边界、overlay 自身 DPI；支持负坐标、混合缩放、最大化、跨屏及拓扑变化，不盲用目标 DPI。
- **D-07:** 顶边外侧优先，保留系统标题按钮区域；设置提供水平/垂直偏移和恢复默认。自绘标题栏保守降级；全屏、窄窗口或无安全位置隐藏，热键仍可用。

### 配置与交付
- **D-08:** 向 schema 1 增加可选 Pin 字段，旧配置缺字段时默认为关闭；沿用原子保存/冲突回滚，新增控件不破坏旧控件 ID。
- **D-09:** 完成真实点击/焦点与前台竞争测试、跟随和排除验证；多屏/DPI、锁屏、常见程序实测与性能证据不得用单元测试替代。便携包记录未验项。
- **D-10:** 保留原生 C++20/Win32、单消息循环、无注入/跨进程 SetParent/大型运行时；只做 Pin 所需小模块整合，不全面重写 MVP。

### the agent's Discretion
模块边界、颜色、位置安全裕量、事件合并节奏和测试夹具由实施时按研究与验证结果决定。本文计划的数值为初始工程选择，不表示用户已独立验收。
</decisions>

<canonical_refs>
## Canonical References
- `D:/HOPP/download/ping-tool/docs/technical-design.zh-CN.md` — 完整批准方案，第 10 节及权限、配置约束。
- `D:/HOPP/download/ping-tool/.planning/REQUIREMENTS.md` — CFG-08、PIN-01–04。
- `D:/HOPP/download/ping-tool/.planning/ROADMAP.md` — 阶段 4 边界及阶段 5 后续验收。
- `D:/HOPP/download/ping-tool/docs/verification.zh-CN.md` — MVP 回归基线与真实测试缺口。
- `D:/HOPP/download/ping-tool/src/main.cpp`、`src/WindowOps.cpp`、`src/Config.cpp` — 当前宿主、筛选、置顶确认和配置事务。
</canonical_refs>

<specifics>
参考 PinWindow 的活动窗口附近点击入口；实现不复制其框架、后台恢复扫描或代码。v0.1.0 标签固定在 166d09b，源码 main 后续文档提交不改变已发布二进制。
</specifics>

<deferred>
## Deferred Ideas
阶段 5 的完整平台矩阵、两小时稳定性、MSVC/签名发布准备仍按原路线。自动更新、联网、注入、驱动、透明度、画中画不进入本阶段。
</deferred>
