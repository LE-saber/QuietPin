# Architecture
隐藏顶层宿主 + 阻塞消息循环；ConfigStore 管持久化，WindowOps 管筛选与身份，AppHost 管热键、设置、可选提示和托盘。UI 线程不等待目标程序；异步操作有界确认。配置替换与启动项失败时保留/恢复旧状态。关闭 UI 返回后台，退出走完整清理。
