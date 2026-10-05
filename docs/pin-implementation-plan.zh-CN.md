# QuietPin 下一阶段：Pin 按钮技术实现计划

日期：2026-10-05。状态：计划已编写，尚未实施。目标版本：v0.2.0。

## 1. 已发布基线与阶段目标

用户确认 MVP 测试成功。源码已上传 [QuietPin 仓库](https://github.com/LE-saber/QuietPin)，[v0.1.0 发布页](https://github.com/LE-saber/QuietPin/releases/tag/v0.1.0)提供 Windows x64 便携 ZIP 和 SHA-256 文件。标签对应提交 `166d09b`；后续文档提交及开发不移动该标签。

下一阶段增加一个默认关闭的 Pin 按钮。开启后，按钮靠近当前合格活动窗口的顶边，点击切换置顶；窗口移动、缩放、最大化、跨屏时跟随。默认无托盘运行和所有现有管理入口继续可用。

继续使用 C++20 + Win32 + GDI，单消息循环，不引入 GUI 框架、运行时或第三方绘图库。按钮是本进程的独立顶层窗口，不注入、不修改原生标题栏，不建立跨进程父子关系。

本阶段对应 CFG-08、PIN-01、PIN-02、PIN-03、PIN-04。完整平台/长期稳定性验收仍在阶段 5；本阶段必须先完成 Pin 自身的兼容与性能检查。

## 2. 当前源码需要解决的集成点

| 当前实现 | 下一阶段改动 |
| --- | --- |
| `App::toggle` 和 `confirm` 已执行权限/排除筛选与异步状态确认 | Pin 点击复用该路径，确认结果驱动按钮状态；不另写一套置顶逻辑 |
| 前台 hook 仅由 `applyTray` 开关；`recent` 会保留部分 Shell 过渡前的目标 | 提取前台事件生命周期，订阅由 Tray/Pin 两个显式需求控制；Pin 每次读取真正前台，不用 `recent` |
| `Identity` 包含 HWND、PID、TID、进程创建时间 | Pin 另加绑定代次与手势快照；destroy/切换使手势失效，避免陈旧消息操作新绑定 |
| Settings 尚无 Pin 字段，设置有“后续提供”说明 | 添加开关、位置偏移、恢复默认；保留已有控件 ID，沿用原子保存事务 |
| manifest 已声明 PerMonitorV2 | overlay 自身处理 DPI 切换；定位计算与绘制尺寸分开，测试跨 DPI-aware 类型目标 |

`Identity` 的进程创建时间不能区分同一进程内 HWND 的复用。绑定代次和销毁通知补充防护，但 Windows 没有公开的原子窗口寿命凭证；不能把多次检查描述为绝对消除了所有竞争。执行前最后再次验证，遇到不确定条件取消操作。

## 3. 模块与数据流

| 模块 | 职责与边界 |
| --- | --- |
| `WindowEventMonitor.h/.cpp` | 共享前台订阅、目标范围事件、投递合并刷新；RAII 卸载 hook |
| `PinController.h/.cpp` | 绑定身份/代次、显示条件、会话状态、输入快照、布局与操作结果同步 |
| `PinOverlay.h/.cpp` | 单个非激活 popup，GDI 绘制、鼠标手势、tooltip、DPI/主题响应 |
| `PinPlacement.h/.cpp` | 无窗口副作用的纯几何函数；物理矩形、DPI、偏移、保护区输入 → 位置或不可放置 |
| `main.cpp` | 配置/设置和现有 toggle、confirm 的少量接线；退出顺序 |

数据流：系统事件 → 合并刷新消息 → 重新读取前台与策略 → 获取物理边界及 overlay DPI → 安全布局 → 显示/隐藏。点击 → 捕获绑定快照 → 释放时复核 → 既有 toggle → confirm → 更新真实状态与反馈。

全局事件只提示“重新检查”，不能信任消息里早已过时的 HWND 就直接显示。一个待处理刷新标记限制宿主队列；回调不读进程信息、不绘制、不发阻塞消息、不让异常穿越系统回调。

## 4. 非激活点击与状态

窗口使用 `WS_POPUP`，扩展样式含 `WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`。显示和移动均不激活；需要覆盖当前目标时使用 `HWND_TOPMOST`，当前前台条件失效立即隐藏。按钮顶层 Z 序与目标的实际置顶状态是两个概念。

`WM_MOUSEACTIVATE` 返回 `MA_NOACTIVATE`，保留鼠标消息；不能返回 `MA_NOACTIVATEANDEAT`，也不能复制状态提示的点击穿透行为。这一选择依据 [WM_MOUSEACTIVATE 文档](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mouseactivate)。不调用 `SetForegroundWindow`、`SetActiveWindow`、`SetFocus`。

左键按下时记录身份、绑定代次、当时前台及按钮命中区，必要时捕获鼠标。释放时同时要求：仍命中按钮、没有丢失捕获、绑定代次未变、当前前台根窗口仍是该目标、身份和当前策略有效、没有已有置顶请求。否则释放捕获并取消手势。双击按普通按下/释放完整处理，每次完整手势最多切换一次，忙碌期间不排队重复请求。

按钮复用一个 HWND，正常目标切换不反复创建销毁。Pin 关闭时销毁，重新开启时按需创建。初始尺寸 24 DIP，用 GDI 矢量针形和不同轮廓/底色区分状态，不仅依赖颜色或 Emoji。悬停显示“置顶此窗口 / 取消置顶”；绘制资源只在 DPI、主题或状态变化时重建。提供可访问名称并通过系统辅助功能检查；默认原生对象若无法表达名称，增加最小 Win32 辅助功能适配，不引入 UI 框架。

| 控制状态 | 行为 |
| --- | --- |
| Disabled | 无 Pin HWND、专属 hook、跟随 timer |
| Hidden | 已开启但无安全前台目标，仅保留必要订阅 |
| Visible | 绑定当前目标，按真实 `WS_EX_TOPMOST` 绘制 |
| Pressed | 保存手势快照，目标改变即取消 |
| Pending | 请求等待既有确认，禁用重复点击；不提前显示成功 |

热键或托盘切换成功、失败、超时后同步刷新当前 Pin；外部置顶工具的变化通过相关窗口事件及重新激活、悬停/点击前复核。首版不承诺在系统未报告变化时零延迟检测外部修改，不为此开启常驻扫描。

## 5. 事件、资源和失效规则

共享前台 hook 按 `tray || pin` 保留：关闭 Pin 不影响 Tray，关闭 Tray 不影响 Pin，两者均关闭时卸载。共享前台订阅不盲目跳过本进程事件，因为设置激活也是必须隐藏 Pin 的条件。

Pin 启用时为目标设置窄范围的 `WINEVENT_OUTOFCONTEXT` hook；订阅 foreground、location change、move/size start/end、minimize start/end、hide/show、destroy、状态相关事件。对象事件限定当前目标 PID，回调进一步核对 HWND、`OBJID_WINDOW`、`CHILDID_SELF`；系统事件采用独立过滤规则，不能照搬对象过滤丢掉事件。不订阅 EVENT_MIN–EVENT_MAX。事件表可拆成数个精确范围，不为了少一个 hook 接收大量无关事件。[SetWinEventHook 文档](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwineventhook)要求注册线程有消息循环，并提醒回调可能重入。

几何刷新以约 16–33 ms 合并，位置/尺寸/状态不变时不调用 `SetWindowPos` 或重绘。只在 move/size 过程使用 33 ms 临时补偿；结束、隐藏、解绑、关闭均停掉。为遗漏结束事件设置最长 5 秒的有界安全期限，到期停止补偿并保守隐藏，下一次可靠事件重新建立状态。动画结束若需补偿最多 3 次短延迟刷新，不能演变成永久恢复扫描。

一次严格失效流程：取消输入捕获 → 隐藏 → 增加绑定代次 → 清空目标 → 停止目标计时器 → 卸载目标 hook。解绑后已排队消息只能触发重新读取前台，不能恢复旧身份。hook 创建失败时保持热键能力，在设置记录失败，Pin 安全隐藏；不无限重试。

以下情况执行失效或重新核对：前台变成其他应用、本工具设置、桌面/Shell；目标销毁、隐藏、最小化、排除、较高权限；DWM cloaked；会话锁定/切换、显示器拓扑变化、退出。会话通知使用隐藏顶层宿主注册 WTS 当前会话通知；锁定先隐藏，解锁再验证前台，注册失败时保守停用 Pin 并记录原因。启停配对注册/注销，宿主销毁前注销。使用 Windows 自带 Wtsapi32，不增加后台线程，依据 [WTS 会话通知文档](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsregistersessionnotification)。cloaking 变化还需可见性事件和恢复刷新，不能只依赖一次前台变化。

## 6. 物理坐标、DPI 与安全位置

定位优先取 `DWMWA_EXTENDED_FRAME_BOUNDS` 的物理屏幕矩形；失败才在已声明 PMv2 的本线程调用 `GetWindowRect`。后者可能包含不可见边框，因此回退路径使用保守安全裕量；不重复缩放 DWM 矩形。[GetWindowRect 文档](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect)明确区分两者的 DPI 行为。

用 `MonitorFromWindow` 和 `GetMonitorInfo` 取目标主所在显示器的 `rcMonitor`/`rcWork`。采用有符号坐标，保留负值；不拿主屏宽高裁剪，也不把最大化工作区填满误判为全屏。跨屏时按钮选择目标所在显示器，计算完成后还要核对按钮实际所在显示器。

不能直接使用 `GetDpiForWindow(target)` 计算按钮大小：该值依目标 DPI awareness 而变化，旧应用可能返回 96 或系统 DPI。先将**隐藏的本工具 PMv2 overlay** 非激活地移到所选显示器内部，获取自身 DPI，再按 `MulDiv(dip, dpi, 96)` 换算尺寸、间距、偏移。`WM_DPICHANGED` 更新 DPI/资源并安排一次锚点布局，以建议矩形作为过渡；最多有界二次收敛，避免位置在显示器边缘振荡。依据 [GetDpiForWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow)，不在 PM-aware 线程盲用 [GetDpiForMonitor](https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor)。

初始布局候选规则：

1. 优先放在顶边外侧、距右边约 160 DIP 处，顶边间距 4 DIP。整个按钮必须在工作区内且仍靠近目标。
2. 外侧无空间时，传统标题栏可尝试同一水平位置的顶部内侧；保护左右系统区域，按钮不得覆盖最小化/最大化/关闭和系统菜单。使用有效的 DWM caption-button 矩形补充保护区；该矩形是窗口相对坐标，必须从原窗口物理原点转换，不能直接加可见框原点。查询失败用保守保护宽度。
3. 自绘标题栏（包括有 WS_CAPTION 的浏览器）不能仅靠样式判断。候选点必须经有超时的 `WM_NCHITTEST` 得到 `HTCAPTION` 等允许结果；`HTCLIENT`/系统按钮/超时拒绝内部候选。需要探测按钮覆盖范围多个点，不能用中心点通过就宣布矩形安全。只在候选布局变化时做探测，总预算不超过 50 ms；使用 `SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT`，不使用可能延长等待的 NOTIMEOUTIFNOTHUNG。该策略依据 [SendMessageTimeout 文档](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw)，仍不能保证了解所有私有布局。
4. 对自绘窗口优先外侧；内部探测不安全时隐藏。用户可调偏移再寻找安全位置。宽度不足、矩形异常、全屏/无安全区域时隐藏，仍支持热键。

用户偏移加到候选锚点上，但不绕过安全区检查；不把无效位置强行 clamp 到关闭按钮或浏览器标签上。窗口显示器边缘处可尝试另一候选，最终仍无安全位置则隐藏。全屏按监视器边界与窗口类型组合保守识别，接受少量误隐藏；最大化普通标题窗口应按上述安全候选显示。纯几何函数输出不可放置原因，便于测试和设置解释。

## 7. 配置与设置事务

保持 schema 1，增加向后兼容可选字段：

```ini
[General]
ShowPin=0

[Pin]
SizeDip=24
OffsetXDip=0
OffsetYDip=0
```

旧 v0.1 配置缺失以上字段时采用默认值，不认定为损坏。首版尺寸固定 24 DIP；字段保留但只接受有效尺寸边界，是否开放尺寸控件后续按实测决定。水平偏移范围初设 −512～512 DIP，垂直 −256～256 DIP，支持负数；解析溢出、非布尔开关及越界按现有损坏配置流程处理，先备份，不静默覆盖。

设置增加“显示 Pin 按钮”、水平/垂直偏移、“恢复默认位置”。关闭时禁用偏移编辑；中英文本同步。新 Control ID 追加到旧 ID 后，避免破坏现有真实设置集成测试。保存成功后才应用运行状态；关闭立即释放，启用失败保存用户意图但报告实际未启用，不虚报完整生效。不变更默认托盘、登录启动与热键。

v0.1 读取新配置会忽略未知字段，但再次保存会丢弃 Pin 字段。因此回退旧版时先备份配置；下一次 v0.2 读到缺失字段会回到关闭。不能承诺跨版本编辑无损。

## 8. GSD 实施顺序

| 计划 | 交付能力 | 主要工作 | 完成条件 |
| --- | --- | --- | --- |
| 04-01 / Wave 1 | 普通窗口可用 Pin 的完整最小路径 | 配置开关、共享前台订阅、单 overlay、点击快照、既有确认接线 | 真实鼠标点击两次切换、焦点保持；旧配置关闭、默认无 overlay；旧 MVP 回归通过 |
| 04-02 / Wave 2，依赖 04-01 | 窗口和屏幕变化下正确跟随 | 几何函数、目标事件、DPI、偏移设置、标题安全区、会话/失效处理 | 关闭/最小化/快速切换无残留；负坐标/不同 DPI 数学测试；混合 DPI 实机检查记录 |
| 04-03 / Wave 3，依赖 04-02 | 可交付 v0.2 候选版 | 辅助功能/主题收尾、竞争/性能测试、应用矩阵、便携包文档 | 必测自动测试通过，实际点击和性能证据齐全；硬件不足项明确待验，不标为通过 |

实施使用顺序执行，每个计划保留提交与 SUMMARY；失败修正后再推进。第一步先把普通窗口全路径做通，再扩展复杂跟随。每步均可关闭 Pin 回到原后台模式；发布前始终可从 v0.1.0 标签回归。

## 9. 验证与发布门槛

**自动测试**：沿用现有 CTest，增加纯几何/配置测试及独立配置的真实 Win32 夹具。覆盖默认关闭、旧配置升级、位置负数/越界、100/125/150/200% 换算、工作区边界、标题保护、无安全候选；真实鼠标 SendInput 点击、前台和焦点句柄前后一致、异步失败不变成功、按下后切换目标/销毁/排除取消、旧排队事件丢弃、最小化还原、程序式移动/最大化、Pin/Tray 开关四种组合。测试创建自身窗口，不操作用户现有窗口或启动项。

**人工测试**：记事本、Explorer、Chrome/Edge、一个 Electron 应用，逐个记录版本、标题栏情况和 Pin 显示/隐藏原因；真实鼠标拖动、双击、贴边/最大化；100% 与 150/200% 双屏（含左侧负坐标），DPI-unaware/system-aware/PMv2 夹具，跨屏来回 20 次；主屏改变/热插拔；锁屏恢复、虚拟桌面、全屏、实际管理员窗口、Tooltip 不激活/不遮挡。实际混合 DPI 环境缺失时，PIN-03 保持未完成，仅发布明确边界的候选版。

**资源测试**：分别测 Pin/Tray 均关闭、Pin 静止显示、持续移动、反复启停 100 次。记录 CPU 时间增量、Private Bytes、Working Set、线程/句柄/GDI/USER 对象与布局次数。关闭状态不得出现 Pin 专属跟随 timer；静止无事件时不得周期扫描或重绘。初始预算：EXE 不超过 2 MiB、静止额外 Private Bytes ≤5 MiB、60 秒空闲 CPU 时间增量 ≤0.1 秒（同机同方法）；预算是待验证目标。对象计数稳定、没有线性泄漏；事件延迟通常 ≤100 ms，拖动误差以视频/采样证据记录，不承诺实时调度。

全部完成后生成 v0.2.0 包和新的验证文档；签名/MSVC 的未验状态继续明确。实际二进制变更后才构建、测试并发布，不把本次计划视作 Pin 已完成。

## 10. 参考与计划审查

已重新检查 [PinWindow 交互与源码](https://github.com/helliong/pinWindow)及 [PowerToys Always On Top](https://learn.microsoft.com/en-us/windows/powertoys/always-on-top)。PinWindow 的标题附近入口、状态/位置配置有参考价值；其恢复轮询和托盘依赖不作为 QuietPin 的默认策略。PowerToys 的快捷键切换、排除及反馈用于核对交互。API 先用 Context7 按非激活、事件、DPI 概念查询，再用上述 Microsoft 原文复核。

GSD 决策/需求覆盖与计划结构将由工具检查；按照项目顺序执行要求，本次研究与计划审查在当前会话完成，未声称通过独立子代理或外部评审。计划验收和未来运行验收分开记录。
