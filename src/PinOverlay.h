#pragma once
#include <windows.h>
#include <functional>
#include <string>

namespace qp {
class PinOverlay {
public:
    ~PinOverlay();
    bool create(HWND host,UINT refreshMessage);
    void destroy();
    void hide();
    void cancel();
    void show(const RECT& rect,bool pinned,bool busy,bool chinese);
    void setActions(std::function<void()> press,std::function<void(bool)> release,std::function<void()> hover);
    HWND window() const { return window_; }
    bool pressed() const { return pressed_; }
private:
    static LRESULT CALLBACK proc(HWND,UINT,WPARAM,LPARAM) noexcept;
    LRESULT message(UINT,WPARAM,LPARAM);
    void paint();
    void name();
    HWND window_=nullptr,tooltip_=nullptr,host_=nullptr;
    UINT refreshMessage_=0;
    bool pinned_=false,busy_=false,chinese_=true,pressed_=false,hover_=false;
    std::wstring label_;
    std::function<void()> press_,hoverAction_;
    std::function<void(bool)> release_;
};
}
