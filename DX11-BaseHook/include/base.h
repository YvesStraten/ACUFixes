#pragma once
#ifndef BASE_H
#define BASE_H

#pragma comment(lib, "d3d11.lib")

#include "libmem++/old-libmem-platform-defines.h"
#include <Windows.h>
struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;

#if defined(MEM_86)
#define WNDPROC_INDEX GWL_WNDPROC
#define PRESENT_CALL __stdcall
#elif defined(MEM_64)
#define WNDPROC_INDEX GWLP_WNDPROC
#define PRESENT_CALL __fastcall
#endif
#define D3D11DEV_LEN 40
#define D3D11SC_LEN  18
#define D3D11CTX_LEN 108

typedef HRESULT(PRESENT_CALL* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef LRESULT(CALLBACK*  WndProc_t) (HWND, UINT, WPARAM, LPARAM);

namespace Base
{
	class Settings;
	void Start(Base::Settings& settings);
	bool Detach();

    // To be implemented by user.
    void ImGuiLayer_WhenMenuIsOpen();
    void ImGuiLayer_EvenWhenMenuIsClosed();
    extern Settings* g_Settings;

    // Information like Swapchain address, original WndProc address
    // helps to transition to outer hooks.
    void ImGuiDrawBasehookDebug();

    using voidptr_t = void*;
    using size_t = ::size_t;
	namespace Data
	{
		extern HMODULE                 thisDLLModule;
		extern ID3D11Device*           pDxDevice11;
		extern IDXGISwapChain*         pSwapChain;
		extern ID3D11DeviceContext*    pContext;
		extern ID3D11RenderTargetView* pMainRenderTargetView;
		extern void*                   pDeviceTable   [D3D11DEV_LEN];
		extern void*                   pSwapChainTable[D3D11SC_LEN];
		extern void*                   pContextTable  [D3D11CTX_LEN];
		extern HWND                    hWindow;
		extern voidptr_t               pPresent;
		extern Present_t               oPresent;
		extern WndProc_t               oWndProc;
		extern size_t                  szPresent;
		extern UINT                    WmKeys[0xFF];
		extern bool                    Detached;
		extern bool                    IsImGuiInitialized;
		extern bool                    ShowMenu;
		extern bool                    IsUsingPresentInnerHook;
		extern Base::Settings*         g_Settings;

		namespace Keys
		{
			extern UINT ToggleMenu;
			extern UINT DetachDll;
		}
	}

	namespace Hooks
	{
		bool Init(bool usePresentInnerHook);
		bool Shutdown();
		HRESULT PRESENT_CALL Present(IDXGISwapChain* thisptr, UINT SyncInterval, UINT Flags);
		LRESULT CALLBACK  WndProc_BasehookControlsThenForwardToImGuiAndThenToOriginal(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // This is called in Hooks::Present() if the "inner" hook is activated.
        // If you want to use an "outer" hook (at the call site), just call this function from there.
        void GrabGraphicsDevicesInitializeImGuiAndDraw(IDXGISwapChain* thisptr);
	}
}

namespace Base
{
class Settings
{
public:
    const bool m_ShouldUsePresentInnerHook;
    const WNDPROC m_WndProc;
public:
    Settings(bool shouldUsePresentInnerHook, WNDPROC wndProc = Base::Hooks::WndProc_BasehookControlsThenForwardToImGuiAndThenToOriginal)
        : m_ShouldUsePresentInnerHook(shouldUsePresentInnerHook)
        , m_WndProc(wndProc)
    {}

    virtual void OnBeforeActivate() = 0;
    virtual void OnBeforeDetach() = 0;

    virtual ~Settings() {}
};
class BasehookSettings_PresentHookInner : public Base::Settings
{
    const bool m_ShowMenuByDefault;
public:
    BasehookSettings_PresentHookInner(bool showMenuByDefault, WNDPROC wndProc = Base::Hooks::WndProc_BasehookControlsThenForwardToImGuiAndThenToOriginal) : Settings(true, wndProc), m_ShowMenuByDefault(showMenuByDefault) {}
    virtual void OnBeforeActivate() override { Base::Data::ShowMenu = m_ShowMenuByDefault; }
    virtual void OnBeforeDetach() override {}
}; class BasehookSettings_OnlyWNDPROC : public Base::Settings
{
    const HWND hWindow;
public:
    BasehookSettings_OnlyWNDPROC(HWND hWindow, WNDPROC wndProc) : Settings(false, wndProc), hWindow(hWindow) {}
    virtual void OnBeforeActivate() override {
        Base::Data::hWindow = hWindow;
        Base::Data::oWndProc = (WNDPROC)SetWindowLongPtr(Base::Data::hWindow, GWLP_WNDPROC, (LONG_PTR)m_WndProc);
    }
    virtual void OnBeforeDetach() override {}
};
}

#endif