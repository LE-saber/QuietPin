#pragma once
#include <windows.h>
#include <optional>
#include <vector>

namespace qp {
struct PinGeometry {
    RECT frame{},work{};
    UINT dpi=96,sizeDip=24;
    int offsetX=0,offsetY=0;
    bool insideAllowed=false,fullscreen=false;
    std::optional<RECT> captionButtons;
};
struct PinCandidate { RECT rect{}; bool inside=false; };
std::vector<PinCandidate> pinCandidates(const PinGeometry& geometry);
bool safePinCandidate(const PinGeometry& geometry,const PinCandidate& candidate);
}
