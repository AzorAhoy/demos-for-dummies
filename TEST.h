#include <windows.h>

bool gWindowWantsToQuit = false;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_ESCAPE:
        {
            gWindowWantsToQuit = true;
        } break;
        }
    } break;
    case WM_SYSCOMMAND:
    {
        switch (wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
        {
            return 0;
        }
        }
    } break;
    case WM_CLOSE:
    {
        gWindowWantsToQuit = true;
    } break;
    }

    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

int main()
{
    const char* windowClassName = "DemosForDummiesWindowClass";

    WNDCLASSA windowClass;
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(nullptr);
    windowClass.hIcon = nullptr;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = windowClassName;
    if (!RegisterClassA(&windowClass))
    {
        return 1;
    }

    const int width = 1280;
    const int height = 720;
    const DWORD exStyle = WS_EX_APPWINDOW;
    const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    HWND hWnd = CreateWindowExA(exStyle, windowClassName, "Demos For Dummies", style,
        0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!hWnd)
    {
        return 1;
    }

    //////////////////////////////////////////////////////////////////////////

    while (!gWindowWantsToQuit)
    {
        MSG msg;
        if (PeekMessage(&msg, hWnd, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(hWnd);
    UnregisterClassA(windowClassName, GetModuleHandle(nullptr));

    return 0;
}

