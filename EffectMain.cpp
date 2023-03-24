#include "Effect.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hWnd;
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass();
BOOL                InitInstance(int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

using InitEffectInstFunc = void(*)(Effect_Instance*);
using UninitEffectInstFunc = void(*)();

int EffectMain(int argc, TCHAR** argv)
{
    // Initialize global strings
    _tcscpy_s(szTitle, _T("Effect"));
    _tcscpy_s(szWindowClass, _T("Effect"));
    hInst = GetModuleHandle(nullptr);
    MyRegisterClass();
    CacheModulePath();
	Timer::GlobalInitialize();
    RegisterEffects();
    auto& GlobalData = GetGlobalData();

    BOOL bQuit = FALSE;
    while (!bQuit)
    {
        size_t nSelection = 0;
        do
        {
            if (nSelection > GlobalData.m_vecEffects.size())
                std::cout << "Typed an invalid effect number." << std::endl;
            std::cout << "Please select an effect, or input '0' to quit: " << std::endl;
            for (size_t i = 0, iEnd = GlobalData.m_vecEffects.size(); i < iEnd; ++i)
            {
#if defined(UNICODE) || defined(_UNICODE)
                std::wcout << i + 1 << _T(". ") << GlobalData.m_vecEffects[i] << std::endl;
#else
                std::cout << i + 1 << ". " << GlobalData.m_vecEffects[i] << std::endl;
#endif
            }          
            std::cout << "Your selection: ";
            std::cin >> nSelection;
        } while (nSelection < 0 && nSelection > GlobalData.m_vecEffects.size());

        if (nSelection == 0)
        {
            bQuit = TRUE;
            break;
        }

        String szLibName = GlobalData.m_vecEffects[nSelection - 1] + _T(".dll");
        HMODULE hLib = ::LoadLibrary(szLibName.c_str());
        if (hLib != INVALID_HANDLE_VALUE)
        {          
            InitEffectInstFunc InitFunc = (InitEffectInstFunc)(::GetProcAddress(hLib, "InitializeEffectInstance"));
            UninitEffectInstFunc UninitFunc = (UninitEffectInstFunc)(::GetProcAddress(hLib, "UninitializeEffectInstance"));
            if (InitFunc == nullptr || UninitFunc == nullptr)
                continue;

            std::shared_ptr<Effect_Instance> pEffectInst = std::make_shared<Effect_Instance>();
            (*InitFunc)(&(*pEffectInst));
            g_pCurrentEffectInst = pEffectInst;

            if (!InitInstance(SW_SHOWNORMAL))
                continue;

            MSG msg;
            ZeroMemory(&msg, sizeof(MSG));
            // Main message loop:
            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                TickInput();
                RenderOneFrame();
                g_D3DInterface.m_pSwapChain->Present(0, 0);
            }

            (*(UninitFunc))();
            GlobalEffectReset();
            ::DestroyWindow(hWnd);
            ::FreeLibrary(hLib);
            ::CloseHandle(hLib);
        }
    }   

    return 0;
}

// int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
//                      _In_opt_ HINSTANCE hPrevInstance,
//                      _In_ LPWSTR    lpCmdLine,
//                      _In_ int       nCmdShow)
// {
//     UNREFERENCED_PARAMETER(hPrevInstance);
//     UNREFERENCED_PARAMETER(lpCmdLine);

//     // TODO: Place code here.

    

//     // Perform application initialization:
//     if (!InitInstance(hInstance, nCmdShow))
//     {
//         return FALSE;
//     }

//     MSG msg;
//     ZeroMemory(&msg, sizeof(MSG));

//     XMVECTOR Deter;
//     // Main message loop:
//     while (msg.message != WM_QUIT)
//     {
//         if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
//         {
// 			TranslateMessage(&msg);
// 			DispatchMessage(&msg);
//         }

//         TickInput();

//         g_CameraMatrix = GetCameraMatrix();
//         g_ProjectionMatrix = GetProjectionMatrix();
//         g_CameraProjectionMatrix = XMMatrixMultiply(g_CameraMatrix, g_ProjectionMatrix);
//         g_InvCameraMatrix = XMMatrixInverse(&Deter, g_CameraMatrix);
//         g_InvProjectionMatrix = XMMatrixInverse(&Deter, g_ProjectionMatrix);
//         g_InvCameraProjectionMatrix = XMMatrixMultiply(g_InvProjectionMatrix, g_InvCameraMatrix);

//         XMStoreFloat4x4A(&g_GeomInvBuffer.InvProj, g_InvProjectionMatrix);
//         XMStoreFloat4x4A(&g_GeomInvBuffer.InvViewProj, g_InvCameraProjectionMatrix);

//         D3D11_MAPPED_SUBRESOURCE SubRc;
//         ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
//         g_pImmediateContext->Map(g_pGeomInvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
//         memcpy(SubRc.pData, &g_GeomInvBuffer, sizeof(GeomInvBuffer));
//         g_pImmediateContext->Unmap(g_pGeomInvBuffer, 0);
//         g_pImmediateContext->VSSetConstantBuffers(1, 1, g_pGeomInvBuffer);

//         RenderOneFrame();

//         g_pSwapChain->Present(0, 0);
//     }
    
//     return (int) msg.wParam;
// }



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInst;
    wcex.hIcon          = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(int nCmdShow)
{
    hWnd = ::CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInst, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    if (!CreateDeviceAndImmediateContext(hWnd))
    {
        return FALSE;
    }

    if (!CreateShaders())
    {
        return FALSE;
    }

    if (!LoadResources())
    {
        return FALSE;
    }

    if (!CreateRenderTargets())
    {
        return FALSE;
    }

    if (!CreateConstantBuffers())
    {
        return FALSE;
    }

    if (!CreateBlendStates() || !CreateRasterizerStates() || !CreateDepthStencilStates() || !CreateSamplerStates())
    {
        return FALSE;
    }

    // LoadGeoms();

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        NotifyKeyDown(wParam);
        break;
    case WM_KEYUP:
        NotifyKeyUp(wParam);
        break;
    case WM_LBUTTONDOWN:
        NotifyLButtonDown(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_LBUTTONUP:
        NotifyLButtonUp(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_MOUSEMOVE:
        NotifyMouseMove(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_RBUTTONDOWN:
        NotifyRButtonDown(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_RBUTTONUP:
        NotifyRButtonUp(LOWORD(lParam), HIWORD(lParam));
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

HWND GetHWnd() 
{
    return hWnd;
}