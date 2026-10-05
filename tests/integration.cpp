#include "Config.h"
#include "WindowOps.h"
#include <iostream>
#include <stdexcept>
#include <functional>
#include <fstream>

using namespace qp;
void check(bool value,const char* label) { if(!value) throw std::runtime_error(label); }
void messages() { MSG m{}; while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); } }
bool waitFor(const std::function<bool()>& condition,int timeout=2000) {
    ULONGLONG stop=GetTickCount64()+timeout;
    while(GetTickCount64()<stop) { messages(); if(condition()) return true; Sleep(10); }
    return false;
}
void activateTestWindow(HWND target) {
    // Test-only foreground coordination; the product never attaches input queues.
    DWORD foreground=GetWindowThreadProcessId(GetForegroundWindow(),nullptr);
    bool attached=foreground && foreground!=GetCurrentThreadId() && AttachThreadInput(GetCurrentThreadId(),foreground,TRUE);
    SetForegroundWindow(target); SetFocus(target);
    if(attached) AttachThreadInput(GetCurrentThreadId(),foreground,FALSE);
}
void chord(Hotkey h) {
    std::vector<INPUT> inputs;
    auto key=[&](WORD vk,DWORD flags) { INPUT input{}; input.type=INPUT_KEYBOARD; input.ki.wVk=vk; input.ki.dwFlags=flags; inputs.push_back(input); };
    const std::pair<UINT,WORD> modifiers[]={{MOD_CONTROL,VK_CONTROL},{MOD_ALT,VK_MENU},{MOD_SHIFT,VK_SHIFT},{MOD_WIN,VK_LWIN}};
    for(auto [bit,vk]:modifiers) if(h.modifiers&bit) key(vk,0);
    key(static_cast<WORD>(h.key),0); key(static_cast<WORD>(h.key),KEYEVENTF_KEYUP);
    for(auto [bit,vk]:modifiers) if(h.modifiers&bit) key(vk,KEYEVENTF_KEYUP);
    check(SendInput(static_cast<UINT>(inputs.size()),inputs.data(),sizeof(INPUT))==inputs.size(),"send test hotkey input");
}
void capture(HWND window,const std::filesystem::path& path) {
    RECT r{}; GetWindowRect(window,&r); int width=r.right-r.left,height=r.bottom-r.top;
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=width;
    info.bmiHeader.biHeight=-height; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr; HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    auto old=SelectObject(dc,bitmap); PrintWindow(window,dc,0);
    BITMAPFILEHEADER header{}; header.bfType=0x4d42; header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
    header.bfSize=header.bfOffBits+width*height*4;
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(&header),sizeof(header));
    file.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(BITMAPINFOHEADER));
    file.write(static_cast<const char*>(bits),width*height*4);
    SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(nullptr,screen);
}
HWND windowFor(DWORD pid,const wchar_t* className) {
    struct Search { DWORD pid; const wchar_t* cls; HWND found; } search{pid,className,nullptr};
    EnumWindows([](HWND hwnd,LPARAM p)->BOOL {
        auto& search=*reinterpret_cast<Search*>(p); DWORD pid=0; GetWindowThreadProcessId(hwnd,&pid);
        wchar_t cls[128]{}; GetClassNameW(hwnd,cls,128);
        if(pid==search.pid && wcscmp(cls,search.cls)==0) { search.found=hwnd; return FALSE; }
        return TRUE;
    },reinterpret_cast<LPARAM>(&search));
    return search.found;
}
PROCESS_INFORMATION launch(const std::wstring& exe,const std::wstring& args) {
    auto command=L"\""+exe+L"\" "+args;
    STARTUPINFOW start{sizeof(start)}; PROCESS_INFORMATION process{};
    check(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&start,&process)!=0,"launch app");
    CloseHandle(process.hThread); return process;
}
LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) { return DefWindowProcW(hwnd,msg,w,l); }
int main(int count,char** args) {
    HANDLE running=nullptr; HWND target=nullptr,host=nullptr; std::filesystem::path directory;
    HWND previous=GetForegroundWindow();
    try {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        check(count==2,"app path argument");
        std::wstring exe=std::filesystem::path(args[1]).wstring();
        directory=std::filesystem::temp_directory_path()/(L"QuietPin-integration-"+std::to_wstring(GetCurrentProcessId()));
        Settings settings; settings.toggle={MOD_CONTROL|MOD_ALT|MOD_SHIFT,VK_F11}; settings.chinese=false;
        std::wstring error; check(saveSettings(directory/L"settings.ini",settings,error),"seed isolated settings");
        auto options=L"--config-dir \""+directory.wstring()+L"\"";
        WNDCLASSW cls{}; cls.lpfnWndProc=proc; cls.hInstance=GetModuleHandleW(nullptr); cls.lpszClassName=L"QuietPin.TestTarget";
        RegisterClassW(&cls);
        target=CreateWindowW(cls.lpszClassName,L"QuietPin automated test window",WS_OVERLAPPEDWINDOW,120,120,420,220,nullptr,nullptr,cls.hInstance,nullptr);
        check(target!=nullptr,"create target"); ShowWindow(target,SW_SHOW); SetForegroundWindow(target);
        auto app=launch(exe,options); running=app.hProcess;
        check(waitFor([&]{host=windowFor(app.dwProcessId,L"QuietPin.Host.v1");return host!=nullptr;}),"hidden host ready");
        check(!IsWindowVisible(host),"host invisible");
        check(windowFor(app.dwProcessId,L"QuietPin.Settings.v1")==nullptr,"no initial settings window");
        activateTestWindow(target); check(waitFor([&]{return GetForegroundWindow()==target;}),"target foreground");
        chord(settings.toggle);
        check(waitFor([&]{return isTopmost(target);}),"pin via application hotkey dispatch");
        check(GetForegroundWindow()==target,"status does not steal foreground");
        HWND status=nullptr;
        check(waitFor([&]{status=windowFor(app.dwProcessId,L"QuietPin.Status.v1");return status && IsWindowVisible(status);}),"status appears after confirmation");
        check(status && (GetWindowLongPtrW(status,GWL_EXSTYLE)&WS_EX_NOACTIVATE),"non activating status window");
        PostMessageW(host,WM_HOTKEY,10,0);
        check(waitFor([&]{return !isTopmost(target);}),"unpin via app");
        auto second=launch(exe,options+L" --settings");
        check(WaitForSingleObject(second.hProcess,3000)==WAIT_OBJECT_0,"second instance exits");
        DWORD secondExit=1; GetExitCodeProcess(second.hProcess,&secondExit); CloseHandle(second.hProcess);
        check(secondExit==0,"second instance delivers settings command");
        HWND ui=nullptr;
        check(waitFor([&]{ui=windowFor(app.dwProcessId,L"QuietPin.Settings.v1");return ui && IsWindowVisible(ui);}),"settings opened on existing instance");
        capture(ui,std::filesystem::path(exe).parent_path()/L"settings-en.bmp");
        // Reserve a new combination externally: save must fail while original toggle remains active.
        constexpr UINT mods=MOD_CONTROL|MOD_SHIFT; constexpr UINT newKey=VK_F9;
        check(RegisterHotKey(nullptr,71,mods|MOD_NOREPEAT,newKey)!=0,"reserve conflicting hotkey");
        SendMessageW(GetDlgItem(ui,100),BM_SETCHECK,BST_CHECKED,0);
        SendMessageW(GetDlgItem(ui,101),BM_SETCHECK,BST_UNCHECKED,0);
        SendMessageW(GetDlgItem(ui,102),BM_SETCHECK,BST_CHECKED,0);
        SendMessageW(GetDlgItem(ui,103),BM_SETCHECK,BST_UNCHECKED,0);
        HWND combo=GetDlgItem(ui,104); int items=static_cast<int>(SendMessageW(combo,CB_GETCOUNT,0,0));
        for(int i=0;i<items;++i) if(SendMessageW(combo,CB_GETITEMDATA,i,0)==newKey) SendMessageW(combo,CB_SETCURSEL,i,0);
        SendMessageW(ui,WM_COMMAND,110,0); // Save
        Settings read; check(loadSettings(directory/L"settings.ini",read,error) && read.toggle==settings.toggle,"conflict keeps persisted shortcut");
        UnregisterHotKey(nullptr,71);
        // Save the now available shortcut, enable tray and disable status; verify persisted settings.
        SendMessageW(GetDlgItem(ui,105),BM_SETCHECK,BST_UNCHECKED,0);
        SendMessageW(GetDlgItem(ui,106),BM_SETCHECK,BST_CHECKED,0);
        SendMessageW(ui,WM_COMMAND,110,0);
        check(loadSettings(directory/L"settings.ini",read,error) && read.toggle==Hotkey{mods,newKey} && read.tray && !read.status,"settings save applies hotkey/tray/status");
        SendMessageW(GetDlgItem(ui,109),WM_SETTEXT,0,reinterpret_cast<LPARAM>(executablePath().c_str()));
        SendMessageW(GetDlgItem(ui,108),CB_SETCURSEL,0,0);
        SendMessageW(ui,WM_COMMAND,110,0);
        check(waitFor([&]{ui=windowFor(app.dwProcessId,L"QuietPin.Settings.v1");return ui && IsWindowVisible(ui);}),"Chinese settings rebuilt");
        check(loadSettings(directory/L"settings.ini",read,error) && read.chinese && read.excluded.size()==1,"language and exclusion persisted");
        capture(ui,std::filesystem::path(exe).parent_path()/L"settings-zh.bmp");
        SendMessageW(ui,WM_CLOSE,0,0); messages();
        activateTestWindow(target);
        chord(read.toggle);
        // Read back the visible settings result instead of depending on a private app state.
        messages(); Sleep(60); messages();
        chord({MOD_CONTROL|MOD_ALT|MOD_SHIFT,'T'});
        check(waitFor([&]{ui=windowFor(app.dwProcessId,L"QuietPin.Settings.v1");return ui && IsWindowVisible(ui);}),"settings shortcut opens without tray interaction");
        wchar_t result[256]{}; SendMessageW(GetDlgItem(ui,114),WM_GETTEXT,256,reinterpret_cast<LPARAM>(result));
        check(!isTopmost(target) && std::wstring(result).find(L"排除")!=std::wstring::npos,"excluded app rejected by actual hotkey");
        SendMessageW(GetDlgItem(ui,109),WM_SETTEXT,0,reinterpret_cast<LPARAM>(L"")); SendMessageW(ui,WM_COMMAND,110,0);
        SendMessageW(ui,WM_CLOSE,0,0); messages();
        check(WaitForSingleObject(running,0)==WAIT_TIMEOUT,"closing settings keeps background alive");
        activateTestWindow(target); check(waitFor([&]{return GetForegroundWindow()==target;}),"target refocused");
        PostMessageW(host,WM_HOTKEY,11,0);
        check(waitFor([&]{return isTopmost(target);}),"new shortcut dispatch pins");
        check(!status || !IsWindowVisible(status),"status disabled");
        auto stop=launch(exe,options+L" --exit");
        check(WaitForSingleObject(stop.hProcess,3000)==WAIT_OBJECT_0,"exit client completes"); CloseHandle(stop.hProcess);
        check(waitFor([&]{return WaitForSingleObject(running,0)==WAIT_OBJECT_0;},3000),"background exits");
        check(!isTopmost(target),"exit restores introduced topmost");
        CloseHandle(running); running=nullptr;
        auto restarted=launch(exe,options); running=restarted.hProcess;
        check(waitFor([&]{host=windowFor(restarted.dwProcessId,L"QuietPin.Host.v1");return host!=nullptr;}),"restart ready with saved settings");
        activateTestWindow(target); chord(read.toggle);
        check(waitFor([&]{return isTopmost(target);}),"persisted hotkey works after restart");
        chord({MOD_CONTROL|MOD_ALT|MOD_SHIFT,'Q'});
        check(waitFor([&]{return WaitForSingleObject(running,0)==WAIT_OBJECT_0;},3000),"exit shortcut stops app");
        check(!isTopmost(target),"exit shortcut cleanup");
        CloseHandle(running); running=nullptr;
        DestroyWindow(target); target=nullptr;
        std::filesystem::remove_all(directory);
        if(previous && IsWindow(previous)) SetForegroundWindow(previous);
        std::cout<<"PASS: hidden startup, hotkey dispatch, focus, toggle, single instance, conflict rollback, saved settings, background close and exit cleanup\n";
        return 0;
    } catch(const std::exception& e) {
        UnregisterHotKey(nullptr,71);
        if(host) PostMessageW(host,WM_APP+1,2,0);
        if(running) {
            waitFor([&]{return WaitForSingleObject(running,0)==WAIT_OBJECT_0;},2000);
            if(WaitForSingleObject(running,0)!=WAIT_OBJECT_0) TerminateProcess(running,1);
            CloseHandle(running);
        }
        if(target) DestroyWindow(target);
        if(previous && IsWindow(previous)) SetForegroundWindow(previous);
        std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1;
    }
}
