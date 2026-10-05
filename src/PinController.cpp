#include "PinController.h"
#include <dwmapi.h>
#include <wtsapi32.h>
#include <algorithm>

namespace qp {
constexpr UINT_PTR MoveTimer=40;
PinController::PinController(HWND host,UINT message,WindowEventMonitor& monitor,const Settings& settings,
    std::function<void(HWND)> toggle,std::function<bool()> busy):host_(host),refreshMessage_(message),monitor_(monitor),settings_(settings),
    toggle_(std::move(toggle)),busy_(std::move(busy)) {
    overlay_.setActions([this]{press();},[this](bool inside){release(inside);},[this]{monitor_.refresh();});
}
PinController::~PinController() { stop(); }
void PinController::clear() {
    ++epoch_; gesture_.reset(); overlay_.hide(); target_.reset(); monitor_.bind(nullptr,0);
    KillTimer(host_,MoveTimer); moving_=false;
}
void PinController::stop() {
    clear(); overlay_.destroy();
    if(sessionRegistered_) { WTSUnRegisterSessionNotification(host_); sessionRegistered_=false; }
}
bool PinController::apply() {
    clear(); error_.clear();
    if(!settings_.pin) { stop(); return true; }
    if(!monitor_.foregroundReady()) { error_=L"Foreground tracking unavailable / 无法跟踪活动窗口"; stop(); return false; }
    if(!sessionRegistered_) sessionRegistered_=WTSRegisterSessionNotification(host_,NOTIFY_FOR_THIS_SESSION)!=0;
    if(!sessionRegistered_ || !overlay_.create(host_,refreshMessage_)) {
        error_=L"Pin could not start safely / Pin 无法安全启动"; stop(); return false;
    }
    monitor_.refresh(); return true;
}
void PinController::suspend(bool value) { suspended_=value; clear(); if(!value) monitor_.refresh(); }
void PinController::press() {
    gesture_.reset();
    if(!target_ || suspended_ || busy_() || !sameWindow(*target_) || GetForegroundWindow()!=target_->hwnd) { overlay_.cancel(); return; }
    gesture_=target_; gestureEpoch_=epoch_;
}
void PinController::release(bool inside) {
    auto gesture=gesture_; gesture_.reset();
    if(!inside || !gesture || gestureEpoch_!=epoch_ || suspended_ || !settings_.pin || busy_() ||
       GetForegroundWindow()!=gesture->hwnd || !sameWindow(*gesture)) return;
    auto checked=inspectWindow(gesture->hwnd,settings_);
    if(!checked.window || checked.window->pid!=gesture->pid || checked.window->tid!=gesture->tid) { clear(); return; }
    toggle_(gesture->hwnd); monitor_.refresh();
}
bool PinController::layout(RECT& r) {
    // Wave 1 conservative tracer: only outside the target, using overlay's own DPI.
    RECT frame{};
    if(FAILED(DwmGetWindowAttribute(target_->hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&frame,sizeof(frame))) && !GetWindowRect(target_->hwnd,&frame)) return false;
    MONITORINFO info{sizeof(info),{},{},0}; if(!GetMonitorInfoW(MonitorFromWindow(target_->hwnd,MONITOR_DEFAULTTONEAREST),&info)) return false;
    if(MonitorFromWindow(overlay_.window(),MONITOR_DEFAULTTONEAREST)!=MonitorFromWindow(target_->hwnd,MONITOR_DEFAULTTONEAREST)) {
        overlay_.hide(); SetWindowPos(overlay_.window(),nullptr,info.rcWork.left+8,info.rcWork.top+8,1,1,SWP_NOZORDER|SWP_NOACTIVATE);
    }
    UINT dpi=GetDpiForWindow(overlay_.window()); if(!dpi) return false;
    int size=MulDiv(settings_.pinSize,dpi,96),gap=MulDiv(4,dpi,96);
    int x=frame.right-MulDiv(160,dpi,96)+MulDiv(settings_.pinOffsetX,dpi,96);
    int y=frame.top-gap-size+MulDiv(settings_.pinOffsetY,dpi,96);
    r={x,y,x+size,y+size};
    return frame.right-frame.left>=MulDiv(220,dpi,96) && r.left>=info.rcWork.left && r.right<=info.rcWork.right &&
        r.top>=info.rcWork.top && r.bottom<=frame.top && r.bottom<=info.rcWork.bottom;
}
void PinController::refresh(bool invalidate,bool moving) {
    if(invalidate) clear();
    if(!settings_.pin || suspended_ || !overlay_.window()) { clear(); return; }
    HWND foreground=GetForegroundWindow();
    if(!target_ || target_->hwnd!=foreground || !sameWindow(*target_)) {
        clear(); auto checked=inspectWindow(foreground,settings_);
        if(!checked.window) return;
        target_=checked.window;
        if(!monitor_.bind(target_->hwnd,target_->pid)) { error_=L"Pin target tracking failed / Pin 窗口跟踪失败"; clear(); return; }
    } else if(!inspectWindow(foreground,settings_).window) { clear(); return; }
    if(moving && !moving_) { moving_=true; moveDeadline_=GetTickCount64()+5000; SetTimer(host_,MoveTimer,33,nullptr); }
    else if(!moving && moving_) { moving_=false; KillTimer(host_,MoveTimer); }
    RECT r{};
    if(!layout(r) || GetForegroundWindow()!=target_->hwnd) { overlay_.hide(); return; }
    overlay_.show(r,isTopmost(target_->hwnd),busy_(),settings_.chinese);
}
void PinController::timer() {
    if(!moving_ || GetTickCount64()>=moveDeadline_) { clear(); return; }
    refresh(false,true);
}
}
