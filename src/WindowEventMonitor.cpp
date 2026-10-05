#include "WindowEventMonitor.h"
#include <utility>

namespace qp {
WindowEventMonitor::~WindowEventMonitor() { stop(); }
bool WindowEventMonitor::observeForeground(HWND host,UINT message,bool enabled) {
    host_=host; message_=message;
    if(enabled) {
        current_=this;
        if(!foreground_) foreground_=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,callback,0,0,WINEVENT_OUTOFCONTEXT);
        return foreground_!=nullptr;
    }
    stop(); return true;
}
void WindowEventMonitor::clearTarget() {
    target_=nullptr;
    for(auto& hook:targetHooks_) { if(hook) UnhookWinEvent(hook); hook=nullptr; }
    events_.moving=false;
}
bool WindowEventMonitor::bind(HWND target,DWORD pid) {
    if(target==target_) return true;
    clearTarget();
    if(!target) return true;
    target_=target;
    constexpr std::array<std::pair<DWORD,DWORD>,7> ranges{{
        {EVENT_OBJECT_DESTROY,EVENT_OBJECT_DESTROY},{EVENT_OBJECT_SHOW,EVENT_OBJECT_HIDE},
        {EVENT_OBJECT_STATECHANGE,EVENT_OBJECT_STATECHANGE},{EVENT_OBJECT_LOCATIONCHANGE,EVENT_OBJECT_LOCATIONCHANGE},
        {EVENT_OBJECT_CLOAKED,EVENT_OBJECT_UNCLOAKED},{EVENT_SYSTEM_MOVESIZESTART,EVENT_SYSTEM_MOVESIZEEND},
        {EVENT_SYSTEM_MINIMIZESTART,EVENT_SYSTEM_MINIMIZEEND}}};
    for(size_t i=0;i<ranges.size();++i) {
        targetHooks_[i]=SetWinEventHook(ranges[i].first,ranges[i].second,nullptr,callback,pid,0,WINEVENT_OUTOFCONTEXT);
        if(!targetHooks_[i]) { clearTarget(); return false; }
    }
    return true;
}
void WindowEventMonitor::stop() {
    clearTarget();
    if(foreground_) { UnhookWinEvent(foreground_); foreground_=nullptr; }
    if(current_==this) current_=nullptr;
    events_={}; posted_=false;
}
void WindowEventMonitor::post() {
    if(!posted_ && host_) posted_=PostMessageW(host_,message_,0,0)!=0;
}
void WindowEventMonitor::refresh() { post(); }
WindowEventMonitor::Events WindowEventMonitor::take() {
    auto result=events_;
    events_.foreground=false; events_.invalidated=false; events_.latestForeground=nullptr;
    posted_=false; return result;
}
void CALLBACK WindowEventMonitor::callback(HWINEVENTHOOK hook,DWORD event,HWND hwnd,LONG object,LONG child,DWORD,DWORD) noexcept {
    auto* self=current_;
    if(!self) return;
    if(hook==self->foreground_ && event==EVENT_SYSTEM_FOREGROUND) {
        self->events_.foreground=true; self->events_.latestForeground=hwnd; self->post(); return;
    }
    bool ours=false;
    for(auto targetHook:self->targetHooks_) if(hook && hook==targetHook) { ours=true; break; }
    if(!ours || !hwnd || hwnd!=self->target_) return;
    if(event>=EVENT_OBJECT_CREATE && (object!=OBJID_WINDOW || child!=CHILDID_SELF)) return;
    if(event==EVENT_SYSTEM_MOVESIZESTART) self->events_.moving=true;
    if(event==EVENT_SYSTEM_MOVESIZEEND) self->events_.moving=false;
    if(event==EVENT_OBJECT_DESTROY || event==EVENT_OBJECT_HIDE || event==EVENT_OBJECT_CLOAKED || event==EVENT_SYSTEM_MINIMIZESTART) {
        self->events_.invalidated=true; self->events_.moving=false;
    }
    self->post();
}
}
