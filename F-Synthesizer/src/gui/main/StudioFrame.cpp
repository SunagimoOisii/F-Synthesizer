#include "StudioFrame.h"
#include "StudioWidgets.h"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <stdexcept>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

namespace studio
{
struct WindowFrame::Native
{
    GLFWwindow* window;
    HWND handle;
    float dragRight = 0;
    bool popupOpen = false;

    static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wp, LPARAM lp,
                                      UINT_PTR id, DWORD_PTR data)
    {
        auto& self = *reinterpret_cast<Native*>(data);
        if (message == WM_NCCALCSIZE)
        {
            // Preserve normal window styles/commands while making the frame client area.
            RECT& rect = wp ? reinterpret_cast<NCCALCSIZE_PARAMS*>(lp)->rgrc[0]
                            : *reinterpret_cast<RECT*>(lp);
            if (IsZoomed(hwnd))
            {
                MONITORINFO monitor{sizeof(monitor)};
                if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
                    rect = monitor.rcWork;
            }
            return 0;
        }
        if (message == WM_GETMINMAXINFO)
        {
            const auto result = DefSubclassProc(hwnd, message, wp, lp);
            auto& limits = *reinterpret_cast<MINMAXINFO*>(lp);
            limits.ptMinTrackSize = {minWidth, minHeight};
            return result;
        }
        if (message == WM_NCHITTEST)
        {
            POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &point);
            RECT client{};
            GetClientRect(hwnd, &client);
            if (!IsZoomed(hwnd))
            {
                const UINT dpi = GetDpiForWindow(hwnd);
                const int border = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) +
                                   GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
                const bool left = point.x < border, right = point.x >= client.right - border;
                const bool top = point.y < border, bottom = point.y >= client.bottom - border;
                if (top) return left ? HTTOPLEFT : right ? HTTOPRIGHT : HTTOP;
                if (bottom) return left ? HTBOTTOMLEFT : right ? HTBOTTOMRIGHT : HTBOTTOM;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
            }
            if (!self.popupOpen && point.y >= 0 && point.y < 72 && point.x >= 0 && point.x < self.dragRight)
                return HTCAPTION;
            return HTCLIENT;
        }
        if (message == WM_NCACTIVATE)
            return DefWindowProcW(hwnd, message, wp, -1);
        if (message == WM_NCDESTROY)
            RemoveWindowSubclass(hwnd, procedure, id);
        return DefSubclassProc(hwnd, message, wp, lp);
    }
};

WindowFrame::WindowFrame(GLFWwindow* window) : native(std::make_unique<Native>())
{
    native->window = window;
    native->handle = glfwGetWin32Window(window);
    RECT client{};
    GetClientRect(native->handle, &client);
    if (!SetWindowSubclass(native->handle, Native::procedure, 1, reinterpret_cast<DWORD_PTR>(native.get())))
        throw std::runtime_error("Window frame could not be initialized");
    const BOOL dark = TRUE;
    DwmSetWindowAttribute(native->handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    const DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(native->handle, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    const MARGINS margins{1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(native->handle, &margins);
    SetWindowPos(native->handle, nullptr, 0, 0, std::max(minWidth, static_cast<int>(client.right)),
                 std::max(minHeight, static_cast<int>(client.bottom)),
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

WindowFrame::~WindowFrame()
{
    if (IsWindow(native->handle)) RemoveWindowSubclass(native->handle, Native::procedure, 1);
}

void WindowFrame::resize(int width, int height)
{
    SetWindowPos(native->handle, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void WindowFrame::drawControls(float width)
{
    native->dragRight = width - 805;
    native->popupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    const bool maximized = glfwGetWindowAttrib(native->window, GLFW_MAXIMIZED) == GLFW_TRUE;
    for (int control = 0; control < 3; ++control)
    {
        const float x = width - 126 + control * 36, y = 25;
        const char* label = control == 0 ? "最小化" : control == 1 ? (maximized ? "元のサイズに戻す" : "最大化") : "閉じる";
        ImGui::PushID("window_control");
        ImGui::PushID(control);
        at(x, y);
        const bool clicked = ImGui::InvisibleButton(label, {36, 36}, ImGuiButtonFlags_EnableNav);
        const bool hover = ImGui::IsItemHovered(), focus = ImGui::IsItemFocused();
        if (hover || focus) box(x, y, 36, 36, control == 2 ? color(125, 55, 55) : raised);
        const ImU32 tint = hover || focus ? fg : muted;
        const float cx = x + 18, cy = y + 18;
        if (control == 0) line(cx - 5, cy + 3, cx + 5, cy + 3, tint);
        else if (control == 1)
        {
            if (maximized)
            {
                ImGui::GetWindowDrawList()->AddRect({cx - 2, cy - 6}, {cx + 6, cy + 2}, tint);
                box(cx - 5, cy - 3, 8, 8, hover || focus ? raised : bg);
                ImGui::GetWindowDrawList()->AddRect({cx - 5, cy - 3}, {cx + 3, cy + 5}, tint);
            }
            else ImGui::GetWindowDrawList()->AddRect({cx - 5, cy - 5}, {cx + 5, cy + 5}, tint);
        }
        else
        {
            line(cx - 5, cy - 5, cx + 5, cy + 5, tint);
            line(cx + 5, cy - 5, cx - 5, cy + 5, tint);
        }
        if (hover && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", label);
        if (clicked)
        {
            if (control == 0) glfwIconifyWindow(native->window);
            else if (control == 1)
            {
                if (maximized) glfwRestoreWindow(native->window);
                else glfwMaximizeWindow(native->window);
            }
            else glfwSetWindowShouldClose(native->window, GLFW_TRUE);
        }
        ImGui::PopID();
        ImGui::PopID();
    }
}
}
