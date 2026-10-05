#include "PinPlacement.h"
#include <algorithm>
#include <limits>

namespace qp {
static bool validRect(const RECT& r) {
    // Bound arithmetic as well as rejecting empty/inverted rectangles.
    constexpr LONG limit=1<<24;
    return r.left>=-limit && r.top>=-limit && r.right<=limit && r.bottom<=limit &&
        r.right>r.left && r.bottom>r.top;
}
static bool contains(const RECT& outer,const RECT& inner) {
    return inner.left>=outer.left && inner.top>=outer.top && inner.right<=outer.right && inner.bottom<=outer.bottom;
}
bool safePinCandidate(const PinGeometry& g,const PinCandidate& c) {
    if(g.fullscreen || g.dpi<48 || g.dpi>768 || g.sizeDip<20 || g.sizeDip>48 ||
       !validRect(g.frame) || !validRect(g.work) || !validRect(c.rect)) return false;
    if(!contains(g.work,c.rect)) return false;
    const int gap=MulDiv(4,g.dpi,96);
    if(!c.inside) return c.rect.left>=g.frame.left+gap && c.rect.right<=g.frame.right-gap && c.rect.bottom<=g.frame.top-gap &&
        g.frame.top-c.rect.bottom<=MulDiv(288,g.dpi,96);
    if(!g.insideAllowed || !contains(g.frame,c.rect) || c.rect.top<g.frame.top+gap ||
       c.rect.bottom>g.frame.top+MulDiv(48,g.dpi,96) || c.rect.left<g.frame.left+MulDiv(38,g.dpi,96)) return false;
    RECT protectedArea{};
    if(g.captionButtons && validRect(*g.captionButtons)) {
        protectedArea=*g.captionButtons; InflateRect(&protectedArea,gap,gap);
    } else protectedArea={g.frame.right-MulDiv(150,g.dpi,96),g.frame.top,g.frame.right,g.frame.top+MulDiv(48,g.dpi,96)};
    RECT overlap{}; return !IntersectRect(&overlap,&c.rect,&protectedArea);
}
std::vector<PinCandidate> pinCandidates(const PinGeometry& g) {
    std::vector<PinCandidate> result;
    if(g.fullscreen || g.dpi<48 || g.dpi>768 || g.sizeDip<20 || g.sizeDip>48 ||
       g.offsetX< -512 || g.offsetX>512 || g.offsetY< -256 || g.offsetY>256 ||
       !validRect(g.frame) || !validRect(g.work) || g.frame.right-g.frame.left<MulDiv(220,g.dpi,96)) return result;
    auto px=[&](int dip){return MulDiv(dip,g.dpi,96);};
    const int size=px(g.sizeDip),x=g.frame.right-px(160)-size+px(g.offsetX);
    const int outsideY=g.frame.top-px(4)-size+px(g.offsetY),insideY=g.frame.top+px(4)+px(g.offsetY);
    const PinCandidate candidates[]={{{x,outsideY,x+size,outsideY+size},false},{{x,insideY,x+size,insideY+size},true},
        {{g.frame.left+px(40)+px(g.offsetX),insideY,g.frame.left+px(40)+px(g.offsetX)+size,insideY+size},true}};
    for(const auto& candidate:candidates) if(safePinCandidate(g,candidate)) result.push_back(candidate);
    return result;
}
}
