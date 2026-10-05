#pragma once
#include "WindowOps.h"
#include "WindowEventMonitor.h"
#include "PinOverlay.h"
#include <optional>

namespace qp {
class PinController {
public:
    PinController(HWND host,UINT refreshMessage,WindowEventMonitor& monitor,const Settings& settings,
        std::function<void(HWND)> toggle,std::function<bool()> busy);
    ~PinController();
    bool apply();
    void stop();
    void refresh(bool invalidate=false,bool moving=false);
    void suspend(bool value);
    void timer();
    HWND window() const { return overlay_.window(); }
    const std::wstring& error() const { return error_; }
private:
    void clear();
    void press();
    void release(bool inside);
    bool layout(RECT& rect);
    HWND host_;
    UINT refreshMessage_;
    WindowEventMonitor& monitor_;
    const Settings& settings_;
    std::function<void(HWND)> toggle_;
    std::function<bool()> busy_;
    PinOverlay overlay_;
    std::optional<Identity> target_,gesture_;
    unsigned long long epoch_=0,gestureEpoch_=0;
    bool suspended_=false,sessionRegistered_=false,moving_=false;
    ULONGLONG moveDeadline_=0;
    std::wstring error_;
};
}
