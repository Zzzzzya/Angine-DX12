#include "App/AppRunner.h"

HWND AAppRunner::m_hWnd = nullptr;

// 总体窗口逻辑封装
int AAppRunner::Run(AAppBase *app, HINSTANCE hInstance)
{
    // 初始化窗口
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"AY_DX12";
    RegisterClassEx(&wcex);

    RECT windowRect = {0, 0, static_cast<LONG>(app->GetWidth()), static_cast<LONG>(app->GetHeight())};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hWnd = CreateWindow(
        L"AY_DX12",
        app->GetTitle(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        app);

    if (!m_hWnd)
    {
        return FALSE;
    }

    app->OnInit();

    ShowWindow(m_hWnd, SW_SHOW);

    // Main Sample Loop
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    app->OnDestroy();

    return static_cast<int>(msg.wParam);
}
LRESULT AAppRunner::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AAppBase *app = reinterpret_cast<AAppBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (app)
        {
            app->OnKeyDown(static_cast<UINT8>(wParam));
        }

    case WM_KEYUP:
        if (app)
        {
            app->OnKeyUp(static_cast<UINT8>(wParam));
        }

    case WM_PAINT:
        if (app)
        {
            app->OnUpdate();
            app->OnRender();
        }

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}