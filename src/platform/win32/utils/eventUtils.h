#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <optional>
#include <bit>

namespace utils {
inline void handleMinMaxInfo(HWND hwnd, LPARAM lparam,
                             const std::optional<uint32_t>& minW,
                             const std::optional<uint32_t>& minH,
                             const std::optional<uint32_t>& maxW,
                             const std::optional<uint32_t>& maxH) {
    auto* mmi = std::bit_cast<MINMAXINFO*>(lparam);
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));

    RECT rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    int borderW = rect.right - rect.left;
    int borderH = rect.bottom - rect.top;

    if (minW) mmi->ptMinTrackSize.x = static_cast<LONG>(*minW) + borderW;
    if (minH) mmi->ptMinTrackSize.y = static_cast<LONG>(*minH) + borderH;
    if (maxW) mmi->ptMaxTrackSize.x = static_cast<LONG>(*maxW) + borderW;
    if (maxH) mmi->ptMaxTrackSize.y = static_cast<LONG>(*maxH) + borderH;
}

[[nodiscard]] LRESULT executeNonClientHitTest(std::optional<VeraHitTestRegions> hitTestRegion,HWND hwnd,int screenX,
                                                 int screenY) {
    if (!hitTestRegion.has_value()) {
        return HTCLIENT;
    }

    POINT pt = {.x = screenX, .y = screenY};
    ScreenToClient(hwnd, &pt);

    const int resizeBorderWidth =
        GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    RECT rc{};
    GetClientRect(hwnd, &rc);

    if (pt.y < resizeBorderWidth) {
        if (pt.x < resizeBorderWidth) return HTTOPLEFT;
        if (pt.x >= rc.right - resizeBorderWidth) return HTTOPRIGHT;
        return HTTOP;
    }
    if (pt.y >= rc.bottom - resizeBorderWidth) {
        if (pt.x < resizeBorderWidth) return HTBOTTOMLEFT;
        if (pt.x >= rc.right - resizeBorderWidth) return HTBOTTOMRIGHT;
        return HTBOTTOM;
    }
    if (pt.x < resizeBorderWidth) return HTLEFT;
    if (pt.x >= rc.right - resizeBorderWidth) return HTRIGHT;

    auto isInside = [mouseX = static_cast<int32_t>(pt.x),
                     mouseY = static_cast<int32_t>(pt.y)](
                        const std::optional<VeraRect>& zone) -> bool {
        if (!zone.has_value()) return false;
        const auto& rect = zone.value();
        return mouseX >= static_cast<int32_t>(rect.x) &&
               mouseX < static_cast<int32_t>(rect.x + rect.width) &&
               mouseY >= static_cast<int32_t>(rect.y) &&
               mouseY < static_cast<int32_t>(rect.y + rect.height);
    };

    const auto& regions = hitTestRegion.value();

    if (isInside(regions.closeButton)) return HTCLOSE;
    if (isInside(regions.maximizeButton)) return HTMAXBUTTON;
    if (isInside(regions.minimizeButton)) return HTMINBUTTON;
    if (isInside(regions.dragRegion))
        return HTCAPTION;

    return HTCLIENT;
}

}