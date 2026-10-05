#include "PinOverlay.h"
#include <commctrl.h>
#include <windowsx.h>
#include <oleacc.h>
#include <algorithm>

namespace qp {
// Keep the IID local: some MinGW oleacc import libraries also export this GUID.
static constexpr IID AccessibleId{0x618736e0,0x3c3d,0x11cf,{0x81,0x0c,0x00,0xaa,0x00,0x38,0x9b,0x71}};
PinOverlay::~PinOverlay() { destroy(); }
void PinOverlay::setActions(std::function<void()> press,std::function<void(bool)> release,std::function<void()> hover) {
    press_=std::move(press); release_=std::move(release); hoverAction_=std::move(hover);
}
bool PinOverlay::create(HWND host,UINT message) {
    if(window_) return true;
    WNDCLASSEXW c{}; c.cbSize=sizeof(c); c.hInstance=GetModuleHandleW(nullptr);
    c.lpfnWndProc=proc; c.lpszClassName=L"QuietPin.Pin.v1"; c.hCursor=LoadCursorW(nullptr,IDC_HAND);
    c.style=CS_DBLCLKS;
    if(!RegisterClassExW(&c) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;
    host_=host; refreshMessage_=message;
    window_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TOPMOST,c.lpszClassName,L"QuietPin Pin",WS_POPUP,
        0,0,24,24,nullptr,nullptr,c.hInstance,this);
    if(!window_) return false;
    tooltip_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window_,nullptr,c.hInstance,nullptr);
    if(!tooltip_) { destroy(); return false; }
    SetWindowPos(tooltip_,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    TOOLINFOW tool{}; tool.cbSize=sizeof(tool); tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
    tool.hwnd=window_; tool.uId=reinterpret_cast<UINT_PTR>(window_); tool.lpszText=const_cast<wchar_t*>(L"Pin");
    if(!SendMessageW(tooltip_,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&tool))) { destroy(); return false; }
    SendMessageW(tooltip_,TTM_SETDELAYTIME,TTDT_INITIAL,600);
    name(); return true;
}
void PinOverlay::cancel() {
    pressed_=false;
    if(window_ && GetCapture()==window_) ReleaseCapture();
    if(window_) InvalidateRect(window_,nullptr,FALSE);
}
void PinOverlay::hide() {
    cancel(); hover_=false;
    if(tooltip_) SendMessageW(tooltip_,TTM_POP,0,0);
    if(window_) ShowWindow(window_,SW_HIDE);
}
void PinOverlay::destroy() {
    hide();
    if(tooltip_) { DestroyWindow(tooltip_); tooltip_=nullptr; }
    if(window_) { DestroyWindow(window_); window_=nullptr; }
}
void PinOverlay::name() {
    label_=busy_?(chinese_?L"正在更新置顶状态":L"Updating window state"):
        pinned_?(chinese_?L"取消此窗口置顶":L"Unpin this window"):(chinese_?L"置顶此窗口":L"Pin this window");
    SetWindowTextW(window_,label_.c_str());
    if(tooltip_) {
        TOOLINFOW tool{}; tool.cbSize=sizeof(tool); tool.hwnd=window_; tool.uId=reinterpret_cast<UINT_PTR>(window_);
        tool.lpszText=label_.data(); SendMessageW(tooltip_,TTM_UPDATETIPTEXTW,0,reinterpret_cast<LPARAM>(&tool));
    }
    NotifyWinEvent(EVENT_OBJECT_NAMECHANGE,window_,OBJID_CLIENT,CHILDID_SELF);
}
void PinOverlay::show(const RECT& rect,bool pinned,bool busy,bool chinese) {
    const bool changed=pinned_!=pinned || busy_!=busy || chinese_!=chinese;
    pinned_=pinned; busy_=busy; chinese_=chinese;
    if(changed) name();
    RECT old{}; GetWindowRect(window_,&old);
    if(!EqualRect(&old,&rect) || !IsWindowVisible(window_)) {
        if(!SetWindowPos(window_,HWND_TOPMOST,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,SWP_NOACTIVATE|SWP_SHOWWINDOW)) { hide(); return; }
        ShowWindow(window_,SW_SHOWNOACTIVATE);
    }
    else if(!(GetWindowLongPtrW(window_,GWL_EXSTYLE)&WS_EX_TOPMOST))
        SetWindowPos(window_,HWND_TOPMOST,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
    if(changed) InvalidateRect(window_,nullptr,FALSE);
}
LRESULT CALLBACK PinOverlay::proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) noexcept {
    auto* self=reinterpret_cast<PinOverlay*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE) {
        self=static_cast<PinOverlay*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
        self->window_=hwnd; SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
    }
    try { if(self) return self->message(msg,w,l); }
    catch(...) { if(self) { self->cancel(); ShowWindow(hwnd,SW_HIDE); } return 0; }
    return DefWindowProcW(hwnd,msg,w,l);
}
LRESULT PinOverlay::message(UINT msg,WPARAM w,LPARAM l) {
    switch(msg) {
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_NCHITTEST: return HTCLIENT;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: paint(); return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if(!busy_ && !pressed_) { pressed_=true; SetCapture(window_); if(press_) press_(); InvalidateRect(window_,nullptr,FALSE); } return 0;
    case WM_LBUTTONUP: {
        const bool had=pressed_; RECT r{}; GetClientRect(window_,&r);
        POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)}; bool inside=PtInRect(&r,p)!=0;
        pressed_=false; if(GetCapture()==window_) ReleaseCapture();
        if(had && release_) release_(inside);
        InvalidateRect(window_,nullptr,FALSE); return 0;
    }
    case WM_CAPTURECHANGED: case WM_CANCELMODE:
        if(pressed_) { pressed_=false; if(release_) release_(false); InvalidateRect(window_,nullptr,FALSE); } return 0;
    case WM_MOUSEMOVE:
        if(!hover_) { hover_=true; TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,window_,0}; TrackMouseEvent(&track);
            if(hoverAction_) hoverAction_();
            InvalidateRect(window_,nullptr,FALSE); } return 0;
    case WM_MOUSELEAVE:
        hover_=false;
        if(pressed_) { cancel(); if(release_) release_(false); }
        InvalidateRect(window_,nullptr,FALSE); return 0;
    case WM_DPICHANGED:
        PostMessageW(host_,refreshMessage_,0,0); return 0;
    case WM_THEMECHANGED: case WM_SYSCOLORCHANGE: InvalidateRect(window_,nullptr,FALSE); return 0;
    case WM_GETOBJECT:
        if(static_cast<LONG>(l)==OBJID_CLIENT) {
            IAccessible* object=nullptr;
            if(SUCCEEDED(CreateStdAccessibleObject(window_,OBJID_CLIENT,AccessibleId,reinterpret_cast<void**>(&object)))) {
                auto result=LresultFromObject(AccessibleId,w,object); object->Release(); return result;
            }
        } break;
    }
    return DefWindowProcW(window_,msg,w,l);
}
void PinOverlay::paint() {
    PAINTSTRUCT paint{}; HDC dc=BeginPaint(window_,&paint); RECT r{}; GetClientRect(window_,&r);
    HIGHCONTRASTW contrast{sizeof(contrast),0,nullptr}; SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0);
    const bool high=(contrast.dwFlags&HCF_HIGHCONTRASTON)!=0;
    COLORREF back=high?GetSysColor(COLOR_WINDOW):busy_?RGB(220,222,225):pinned_?RGB(30,102,180):pressed_?RGB(205,220,239):hover_?RGB(226,237,250):RGB(246,248,251);
    COLORREF ink=high?GetSysColor(COLOR_WINDOWTEXT):pinned_?RGB(255,255,255):RGB(35,52,73);
    auto brush=CreateSolidBrush(back); FillRect(dc,&r,brush); DeleteObject(brush);
    auto border=CreateSolidBrush(high?GetSysColor(COLOR_WINDOWTEXT):RGB(140,157,179)); FrameRect(dc,&r,border); DeleteObject(border);
    const int size=std::min(r.right,r.bottom);
    auto scale=[&](int x){return MulDiv(x,size,24);};
    HPEN pen=CreatePen(PS_SOLID,std::max(1,scale(2)),ink); auto oldPen=SelectObject(dc,pen); auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
    if(busy_) {
        for(int i=0;i<3;++i) Ellipse(dc,scale(5+6*i),scale(11),scale(7+6*i),scale(13));
    } else if(pinned_) {
        POINT points[]={{scale(8),scale(4)},{scale(16),scale(4)},{scale(16),scale(9)},{scale(18),scale(12)},
            {scale(6),scale(12)},{scale(8),scale(9)},{scale(8),scale(4)}};
        Polyline(dc,points,7); MoveToEx(dc,scale(12),scale(12),nullptr); LineTo(dc,scale(12),scale(21));
    } else {
        POINT points[]={{scale(11),scale(4)},{scale(18),scale(9)},{scale(15),scale(13)},{scale(15),scale(16)},
            {scale(5),scale(9)},{scale(9),scale(8)},{scale(11),scale(4)}};
        Polyline(dc,points,7); MoveToEx(dc,scale(10),scale(13),nullptr); LineTo(dc,scale(5),scale(20));
    }
    SelectObject(dc,oldBrush); SelectObject(dc,oldPen); DeleteObject(pen); EndPaint(window_,&paint);
}
}
