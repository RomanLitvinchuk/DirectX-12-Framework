#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgi.h>
#include <DirectXHelpers.h>
#include <comdef.h>
#include "window_class.h"
#include <SimpleMath.h>
#include "DX12App.h"
#include "game_timer.h"
#include "vertex.h"
#include "d3dUtil.h"
#pragma comment(linker, "/entry:wWinMainCRTStartup")
HWND g_hWnd = 0;
DX12App MyFramework;

using namespace DirectX;
using namespace DX12;
using namespace Microsoft::WRL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);

        SetWindowLongPtr(hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
        return 0;
    }
    case WM_INPUT:
    {
        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

        BYTE* lpb = new BYTE[dwSize];
        if (lpb == NULL) return 0;

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
            delete[] lpb;
            return 0;
        }

        RAWINPUT* raw = (RAWINPUT*)lpb;

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            short dx = raw->data.mouse.lLastX;
            short dy = raw->data.mouse.lLastY;

            bool leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            MyFramework.GetCamera().UpdateCameraTarget(leftDown ? MK_LBUTTON : 0, dx, dy);
        }

        else if (raw->header.dwType == RIM_TYPEKEYBOARD)
        {
            RAWKEYBOARD& keyboard = raw->data.keyboard;

            UINT virtualKey = keyboard.VKey;
            UINT scanCode = keyboard.MakeCode;
            UINT flags = keyboard.Flags;

            bool keyDown = !(flags & RI_KEY_BREAK);

            if (virtualKey < 256) {
                MyFramework.m_key_states[virtualKey] = keyDown;
            }
            if (virtualKey == 'F' && keyDown) {
                MyFramework.GetCamera().UpdateFrustumCullingState();
                MessageBox(NULL, L"Frustum culling is switched", L"Switch", MB_OK);
            }
        }

        delete[] lpb;
        break;
    }

    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED)
            break;

        int newWidth = LOWORD(lParam);
        int newHeight = HIWORD(lParam);
        MyFramework.SetClientWH(newWidth, newHeight);

        if (MyFramework.IsDeviceCreated())
        {
            MyFramework.OnResize();
        }
        break;
    }

    case WM_LBUTTONDOWN:
        MyFramework.OnMouseDown(hwnd);
        return 0;

    case WM_LBUTTONUP:
        MyFramework.OnMouseUp();
        return 0;


    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_QUIT:
        PostQuitMessage(0);
        break;
    }


    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WindowClass::WRun(GameTimer* gt) {
    MSG msg = {};
    gt->Reset();

    while (msg.message != WM_CLOSE)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            gt->Tick();
            MyFramework.CalculateGameStats(hWnd_);
            MyFramework.Update();
            MyFramework.Draw();
        }
    }

    return static_cast<int>(msg.wParam);
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	WindowClass wnd(hInstance, hPrevInstance);
	wnd.initWindow(WndProc);
	wnd.CheckRegister();
	wnd.CreateWnd(MyFramework.GetClientWidth(), MyFramework.GetClientHeight());
	if (!wnd.CheckCreation()) {
		return 0;
	}
	g_hWnd = wnd.getHWND();

	MyFramework.InitializeDevice();
	MyFramework.InitializeCommandObjects();
	MyFramework.CreateSwapChain(g_hWnd);
	MyFramework.CreateRTVAndDSVDescriptorHeaps();
	MyFramework.CreateRTV();
	MyFramework.CreateDSV();
    MyFramework.LoadTextures();
    MyFramework.CreateCBVDescriptorHeap();
    MyFramework.CreateSRV();
    MyFramework.CreateSamplerHeap();
	MyFramework.SetViewport();
	MyFramework.SetScissor();
	MyFramework.InitProjectionMatrix();
    MyFramework.Parsing();
    MyFramework.BuildOctree();
    MyFramework.CreateSOBuffers();
    MyFramework.BuildBulbGeometry();
    MyFramework.BuildBoxGeometry();
	MyFramework.CreateVertexBuffer();
	MyFramework.CreateIndexBuffer();
	MyFramework.InitUploadBuffers();
    MyFramework.InitUAVBuffers();
    MyFramework.InitEmitter();
	MyFramework.CreateConstantBufferView();
    MyFramework.InitRenderSystem();
    MyFramework.InitShadowMap();


	wnd.RegisterRawInputDevice();
	wnd.ShowWnd();
	wnd.UpdateWnd();
	

	wnd.WRun(&MyFramework.GetTimer());

	return 0;
}