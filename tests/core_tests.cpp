#include "Config.h"
#include "WindowOps.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace qp;
void check(bool condition,const char* name) { if(!condition) throw std::runtime_error(name); }
int main() {
    try {
        check(validHotkey({3,'T'}),"default hotkey");
        check(!validHotkey({0,'T'}) && !validHotkey({16,'T'}) && !validHotkey({3,VK_F12}),"invalid hotkeys");
        check(isExcluded(L"C:\\Apps\\Chrome.EXE",{L"chrome.exe"}),"exact filename case insensitive");
        check(!isExcluded(L"C:\\Apps\\chromedriver.exe",{L"chrome.exe"}),"no substring exclusion");
        check(isExcluded(L"C:\\工具\\app.exe",{L"c:\\工具\\APP.EXE"}),"unicode full path");
        check(!isExcluded(L"D:\\工具\\app.exe",{L"c:\\工具\\app.exe"}),"same name other path");
        auto directory=std::filesystem::temp_directory_path()/(L"QuietPin-core-"+std::to_wstring(GetCurrentProcessId()));
        std::filesystem::create_directories(directory);
        auto file=directory/L"设置.ini";
        Settings original; original.chinese=true; original.tray=true; original.status=false;
        original.toggle={MOD_CONTROL|MOD_SHIFT,VK_F9}; original.excluded={L"C:\\工具 文件\\程序.exe",L"chrome.exe"};
        original.pin=true; original.pinOffsetX=-31; original.pinOffsetY=17;
        std::wstring error;
        check(saveSettings(file,original,error),"atomic config save");
        Settings loaded;
        check(loadSettings(file,loaded,error),"config reload");
        check(loaded.toggle==original.toggle && loaded.excluded==original.excluded && loaded.chinese && loaded.tray && !loaded.status,"roundtrip preserves values");
        check(loaded.pin && loaded.pinSize==24 && loaded.pinOffsetX==-31 && loaded.pinOffsetY==17,"Pin roundtrip");
        int offset=0;
        check(pinOffset(L" -512 ",-512,512,offset) && offset==-512,"signed Pin offset");
        check(!pinOffset(L"-513",-512,512,offset) && !pinOffset(L"999999999999999999",-512,512,offset) &&
              !pinOffset(L"+2",-512,512,offset) && !pinOffset(L"-",-512,512,offset),"reject invalid Pin offsets");
        const auto legacy=directory/L"legacy.ini";
        const std::wstring old=L"\xfeff[General]\r\nSchemaVersion=1\r\nLanguage=en\r\nShowStatus=1\r\nShowTray=0\r\nStartWithWindows=0\r\n[Hotkeys]\r\nToggleModifiers=3\r\nToggleKey=84\r\n[Exclusions]\r\nCount=0\r\n";
        auto writeIni=[&](const std::wstring& data) { std::ofstream out(legacy,std::ios::binary|std::ios::trunc); out.write(reinterpret_cast<const char*>(data.data()),data.size()*sizeof(wchar_t)); };
        writeIni(old); Settings upgraded=original;
        check(loadSettings(legacy,upgraded,error) && !upgraded.pin && upgraded.pinOffsetX==0,"v0.1 config upgrades with Pin off");
        writeIni(old+L"[Pin]\r\nOffsetXDip=-513\r\n");
        check(!loadSettings(legacy,upgraded,error) && !upgraded.pin,"bad Pin config leaves settings unchanged");
        writeIni(old+L"[Pin]\r\nSizeDip=0\r\n");
        check(!loadSettings(legacy,upgraded,error),"invalid Pin size rejected");
        Settings invalid=original; invalid.excluded={L"bad\r\nvalue.exe"};
        check(!saveSettings(file,invalid,error),"reject newline injection");
        check(loadSettings(file,loaded,error) && loaded.excluded==original.excluded,"failed save leaves prior config");
        invalid=original; invalid.pinOffsetY=257;
        check(!saveSettings(file,invalid,error) && loadSettings(file,loaded,error) && loaded.pinOffsetY==17,"bad Pin save keeps prior file");
        // Malformed file must not mutate the caller's accepted settings.
        { std::ofstream stream(file,std::ios::binary|std::ios::trunc); stream<<"broken"; }
        check(!loadSettings(file,loaded,error) && loaded.excluded==original.excluded,"corrupt config rejected without mutation");
        check(inspectWindow(GetDesktopWindow(),original).error==WindowError::System,"desktop protected");
        check(!sameWindow(Identity{}),"invalid identity safe");
        WNDCLASSW cls{}; cls.lpfnWndProc=DefWindowProcW; cls.hInstance=GetModuleHandleW(nullptr); cls.lpszClassName=L"QuietPin.CoreTest";
        RegisterClassW(&cls);
        HWND own=CreateWindowW(cls.lpszClassName,L"Own test window",WS_POPUP|WS_VISIBLE,-10000,-10000,10,10,nullptr,nullptr,cls.hInstance,nullptr);
        check(own && inspectWindow(own,original).error==WindowError::Own,"own windows rejected");
        Identity identity; identity.hwnd=own; identity.tid=GetWindowThreadProcessId(own,&identity.pid);
        FILETIME exited{},kernel{},user{}; GetProcessTimes(GetCurrentProcess(),&identity.created,&exited,&kernel,&user);
        check(sameWindow(identity),"live window identity"); DestroyWindow(own);
        DWORD failure=0;
        check(!sameWindow(identity) && !requestTopmost(identity,true,failure),"destroyed target rejected safely");
        const auto value=L"QuietPin.AutomatedTest."+std::to_wstring(GetCurrentProcessId());
        StartupSnapshot before;
        check(readStartup(value,before,error),"read isolated startup snapshot");
        struct Restore {
            std::wstring value; StartupSnapshot before;
            ~Restore() { std::wstring ignored; restoreStartup(value,before,ignored); }
        } restore{value,before};
        check(writeStartup(value,true,error),"enable isolated startup value");
        StartupSnapshot enabled;
        check(readStartup(value,enabled,error) && startupMatches(enabled),"quoted executable startup entry");
        check(writeStartup(value,false,error),"disable own startup entry");
        StartupSnapshot disabled;
        check(readStartup(value,disabled,error) && !disabled.exists,"own startup entry removed");
        StartupSnapshot foreign; foreign.exists=true; foreign.type=REG_SZ;
        const std::wstring other=L"\"C:\\Other app\\other.exe\" --startup";
        const auto* bytes=reinterpret_cast<const BYTE*>(other.c_str());
        foreign.bytes.assign(bytes,bytes+(other.size()+1)*sizeof(wchar_t));
        check(restoreStartup(value,foreign,error),"seed foreign startup value");
        check(!writeStartup(value,false,error),"refuse to delete another application's startup value");
        StartupSnapshot preserved;
        check(readStartup(value,preserved,error) && preserved.bytes==foreign.bytes,"foreign startup entry preserved");
        std::filesystem::remove_all(directory);
        std::cout<<"PASS: hotkeys, exact exclusions, unicode atomic configuration, corruption, desktop protection and startup ownership\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1; }
}
