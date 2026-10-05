#pragma once
#include <windows.h>
#include <memory>
#include <string>

namespace qp {
inline constexpr wchar_t CurrentVersion[]=L"0.2.0";
inline constexpr wchar_t DownloadPage[]=L"https://github.com/LE-saber/QuietPin/releases/latest";
enum class UpdateStatus { Idle, Checking, Current, NewVersion, DifferentBuild, Unverified, Failed };
struct UpdateResult {
    UpdateStatus status=UpdateStatus::Idle;
    std::wstring version;
};
class UpdateChecker {
public:
    UpdateChecker();
    ~UpdateChecker();
    UpdateChecker(const UpdateChecker&)=delete;
    UpdateChecker& operator=(const UpdateChecker&)=delete;
    void start(HWND notifyWindow,UINT message);
    UpdateResult result() const;
    void stop();
private:
    struct State;
    std::shared_ptr<State> state;
};
}
