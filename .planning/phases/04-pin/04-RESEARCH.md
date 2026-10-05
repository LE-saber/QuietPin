# Phase 4: Pin — Research

**Date:** 2026-10-05
**Scope:** 原生非激活 Pin、事件跟随、几何/DPI 与 MVP 集成。
**Method:** 当前会话顺序研究；Context7 resolve/query Win32 文档 + 官方 API 原文 + 当前源码定位；未派发代理。

## 推荐方案
沿用 C++20/Win32/GDI，拆分 WindowEventMonitor、PinController、PinOverlay、PinPlacement 小模块。完整技术结论及引用见 `docs/pin-implementation-plan.zh-CN.md` 第 2–7、10 节。

## 源码证据
- main.cpp：foregroundHook 目前只在 applyTray 中建立；rememberForeground 使用最近外部目标，不能直接供 Pin 使用。
- main.cpp：toggle/confirm 先筛选再异步请求，500 ms 确认上限；Pin 要复用它，避免点击路径把请求接受视为完成。
- WindowOps.cpp：现有 Identity 做进程/线程/创建时间校验、权限筛选与排除。窗口内句柄复用仍需绑定代次和 destroy 事件取消手势。
- Config.cpp：schema 1 严格解析核心字段、原子保存；Pin 字段必须可选，带符号偏移需独立校验。
- manifest：已有 PMv2；不能因此盲目用目标自身 DPI 作为按钮显示 DPI。

## 已复核 API 结论
1. WM_MOUSEACTIVATE 的 MA_NOACTIVATE 保留点击；ANDEAT 丢弃。Pin 不继承提示穿透策略。
2. OUTOFCONTEXT 回调依赖注册线程消息循环、存在重入，需只投递合并刷新；对象事件与系统事件过滤规则分开。
3. DWM 可见边界是屏幕坐标且不按 DPI 调整；GetWindowRect 可能包含不可见边框与虚拟化。PMv2 overlay 的自身 DPI 是尺寸换算依据。
4. GetDpiForWindow 取决于 HWND 的 DPI awareness；GetDpiForMonitor 不应直接在 PM-aware 线程用作替代。
5. Caption-button bounds 是窗口相对坐标，不可直接与可见框相加；自绘标题必须保守命中探测，不能假定右侧 3 个系统按钮就是全部交互区。

## 风险与对策
- 输入时目标切换：down/up 身份+代次+真实前台再验证，失败取消。
- 事件风暴：目标 PID/HWND/对象过滤、一次待处理消息、布局去重。
- 结束事件遗漏：短期补偿最多 5 秒，到期停掉并隐藏；不长期轮询。
- 关闭一功能卸载另一功能 hook：共享前台需求位；四组合实测。
- 自绘/最大化空间不足：外侧优先、安全内部候选、无位置隐藏，偏移不绕过安全。
- 会话/显示器切换：立即隐藏、取消手势、恢复重采样，注册失败保守隐藏。
- 多屏测试环境不足：数学夹具验证坐标，不等同实机 PIN-03 验收。

## Validation Architecture
现有 CTest core/integration 基础可复用，无需安装测试框架。新增几何纯函数和真实 Pin 点击夹具分别验证坐标与输入/focus/state。每个提交快速测试、每个 wave 全套回归；MVP 测试保持。真实混合 DPI、跨应用标题区、锁屏/热插拔需人工记录；性能按本机 v0.1 方法比较。详细采样契约在 04-VALIDATION.md。

## Open Questions
非阻塞：自绘应用事件完整性与标题安全空间需实测决定兼容降级；固定 24 DIP 是否足够需试用。目标需求已有批准，不重新询问框架。任何兼容补偿须有复現及有界期限。
