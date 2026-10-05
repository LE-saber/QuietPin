#pragma once
#include <windows.h>
#include <array>

namespace qp {
// One monitor on the UI thread; callbacks never perform layout or query processes.
class WindowEventMonitor {
public:
    struct Events { bool foreground=false, invalidated=false, moving=false; HWND latestForeground=nullptr; };
    ~WindowEventMonitor();
    bool observeForeground(HWND host, UINT message, bool enabled);
    bool bind(HWND target, DWORD pid);
    void stop();
    void refresh();
    Events take();
    bool foregroundReady() const { return foreground_!=nullptr; }
private:
    static void CALLBACK callback(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD) noexcept;
    void clearTarget();
    void post();
    inline static WindowEventMonitor* current_=nullptr;
    HWND host_=nullptr,target_=nullptr;
    UINT message_=0;
    HWINEVENTHOOK foreground_=nullptr;
    std::array<HWINEVENTHOOK,7> targetHooks_{};
    Events events_;
    bool posted_=false;
};
}
