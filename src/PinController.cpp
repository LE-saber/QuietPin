#include "PinController.h"
#include "PinPlacement.h"
#include <dwmapi.h>
#include <wtsapi32.h>
#include <algorithm>
#include <limits>

namespace qp {
constexpr UINT_PTR MoveTimer=40;
PinController::PinController(HWND host,UINT message,WindowEventMonitor& monitor,const Settings& settings,
    std::function<void(HWND)> toggle,std::function<bool()> busy):host_(host),refreshMessage_(message),monitor_(monitor),settings_(settings),
    toggle_(std::move(toggle)),busy_(std::move(busy)) {
    overlay_.setActions([this]{press();},[this](bool inside){release(inside);},[this]{probeKnown_=false; monitor_.refresh();});
}
PinController::~PinController() { stop(); }
void PinController::clear() {
    ++epoch_; gesture_.reset(); overlay_.hide(); target_.reset(); monitor_.bind(nullptr,0);
    probeKnown_=false;
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
void PinController::session(bool available) { sessionBlocked_=!available; clear(); if(available) monitor_.refresh(); }
void PinController::press() {
    gesture_.reset();
    if(!target_ || suspended_ || sessionBlocked_ || busy_() || !sameWindow(*target_) || GetForegroundWindow()!=target_->hwnd) { overlay_.cancel(); return; }
    gesture_=target_; gestureEpoch_=epoch_;
}
void PinController::release(bool inside) {
    auto gesture=gesture_; gesture_.reset();
    if(!inside || !gesture || gestureEpoch_!=epoch_ || suspended_ || sessionBlocked_ || !settings_.pin || busy_() ||
       GetForegroundWindow()!=gesture->hwnd || !sameWindow(*gesture)) return;
    auto checked=inspectWindow(gesture->hwnd,settings_);
    if(!checked.window || checked.window->pid!=gesture->pid || checked.window->tid!=gesture->tid) { clear(); return; }
    // A custom title bar can change its hit regions without moving the window.
    // Recheck the actual occupied rectangle at release before performing an action.
    probeKnown_=false;
    RECT safe{},shown{}; GetWindowRect(overlay_.window(),&shown);
    if(!layout(safe) || !EqualRect(&safe,&shown) || GetForegroundWindow()!=gesture->hwnd) { overlay_.hide(); return; }
    toggle_(gesture->hwnd); monitor_.refresh();
}
bool PinController::layout(RECT& r) {
    RECT frame{},windowRect{};
    if(!GetWindowRect(target_->hwnd,&windowRect)) return false;
    if(FAILED(DwmGetWindowAttribute(target_->hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&frame,sizeof(frame)))) frame=windowRect;
    MONITORINFO info{sizeof(info),{},{},0}; if(!GetMonitorInfoW(MonitorFromWindow(target_->hwnd,MONITOR_DEFAULTTONEAREST),&info)) return false;
    if(MonitorFromWindow(overlay_.window(),MONITOR_DEFAULTTONEAREST)!=MonitorFromWindow(target_->hwnd,MONITOR_DEFAULTTONEAREST)) {
        overlay_.hide(); SetWindowPos(overlay_.window(),nullptr,info.rcWork.left+8,info.rcWork.top+8,1,1,SWP_NOZORDER|SWP_NOACTIVATE);
    }
    UINT dpi=GetDpiForWindow(overlay_.window()); if(!dpi) return false;
    const auto style=GetWindowLongPtrW(target_->hwnd,GWL_STYLE);
    bool coversMonitor=frame.left<=info.rcMonitor.left+2 && frame.top<=info.rcMonitor.top+2 &&
        frame.right>=info.rcMonitor.right-2 && frame.bottom>=info.rcMonitor.bottom-2;
    PinGeometry geometry{frame,info.rcWork,dpi,settings_.pinSize,settings_.pinOffsetX,settings_.pinOffsetY,
        (style&WS_CAPTION)==WS_CAPTION,coversMonitor && !(IsZoomed(target_->hwnd) && (style&WS_CAPTION)==WS_CAPTION),std::nullopt};
    RECT buttons{};
    if(SUCCEEDED(DwmGetWindowAttribute(target_->hwnd,DWMWA_CAPTION_BUTTON_BOUNDS,&buttons,sizeof(buttons))) &&
       buttons.right>buttons.left && buttons.bottom>buttons.top) {
        OffsetRect(&buttons,windowRect.left,windowRect.top); geometry.captionButtons=buttons;
    }
    const auto candidates=pinCandidates(geometry);
    const auto deadline=GetTickCount64()+50;
    for(const auto& candidate:candidates) {
        if(candidate.inside) {
            if(!probeKnown_ || !EqualRect(&probeRect_,&candidate.rect)) {
                probeRect_=candidate.rect; probeKnown_=true; probeSafe_=true;
                const RECT& b=candidate.rect;
                const POINT points[]={{b.left,b.top},{b.right-1,b.top},{b.left,b.bottom-1},{b.right-1,b.bottom-1},{(b.left+b.right)/2,(b.top+b.bottom)/2}};
                for(auto point:points) {
                    auto now=GetTickCount64(); DWORD_PTR hit=0;
                    if(now>=deadline || point.x<std::numeric_limits<SHORT>::min() || point.x>std::numeric_limits<SHORT>::max() ||
                       point.y<std::numeric_limits<SHORT>::min() || point.y>std::numeric_limits<SHORT>::max() ||
                       !SendMessageTimeoutW(target_->hwnd,WM_NCHITTEST,0,MAKELPARAM(static_cast<SHORT>(point.x),static_cast<SHORT>(point.y)),
                           SMTO_ABORTIFHUNG|SMTO_BLOCK|SMTO_ERRORONEXIT,static_cast<UINT>(std::min<ULONGLONG>(10,deadline-now)),&hit) || hit!=HTCAPTION) {
                        probeSafe_=false; break;
                    }
                }
            }
            if(!probeSafe_) continue;
        }
        r=candidate.rect; return true;
    }
    return false;
}
void PinController::refresh(bool invalidate,bool moving) {
    if(invalidate) clear();
    if(!settings_.pin || suspended_ || sessionBlocked_ || !overlay_.window()) { clear(); return; }
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
