#include <cstdio>
#include <thread>

#include "hooks.hpp"

#include "backend/dx10/hook_directx10.hpp"
#include "backend/dx11/hook_directx11.hpp"
#include "backend/dx12/hook_directx12.hpp"
#include "backend/dx9/hook_directx9.hpp"

#include "backend/opengl/hook_opengl.hpp"
#include "backend/vulkan/hook_vulkan.hpp"

#include "../console/console.hpp"
#include "../menu/menu.hpp"
#include "../utils/utils.hpp"

#include "../dependencies/imgui/imgui_impl_dx9.h"
#include "../dependencies/imgui/imgui_impl_win32.h"
#include "../dependencies/minhook/MinHook.h"

static HWND g_hWindow = NULL;

static DWORD WINAPI ReinitializeGraphicalHooks(LPVOID lpParam) {
    LOG("[!] Hooks will reinitialize!\n");

    HWND hNewWindow = U::GetProcessWindow( );
    while (hNewWindow == reinterpret_cast<HWND>(lpParam)) {
        hNewWindow = U::GetProcessWindow( );
    }

    H::bShuttingDown = true;

    H::Free( );
    H::Init( );

    H::bShuttingDown = false;
    Menu::bShowMenu = true;

    return 0;
}

static WNDPROC oWndProc;
static LRESULT WINAPI WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_INSERT) {
            Menu::bShowMenu = !Menu::bShowMenu;
            return 0;
        } else if (wParam == VK_HOME) {
            HANDLE hHandle = CreateThread(NULL, 0, ReinitializeGraphicalHooks, NULL, 0, NULL);
            if (hHandle != NULL)
                CloseHandle(hHandle);
            return 0;
        } else if (wParam == VK_END) {
            H::bShuttingDown = true;
            U::UnloadDLL( );
            return 0;
        }
    } else if (uMsg == WM_DESTROY) {
        HANDLE hHandle = CreateThread(NULL, 0, ReinitializeGraphicalHooks, hWnd, 0, NULL);
        if (hHandle != NULL)
            CloseHandle(hHandle);
    }

    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	if (Menu::bShowMenu) {
        ImGuiIO& io = ImGui::GetIO( );

        switch (uMsg) {
            case WM_LBUTTONDOWN:
                io.MouseDown[0] = true;
                break;
            case WM_LBUTTONUP:
                io.MouseDown[0] = false;
                break;
            case WM_RBUTTONDOWN:
                io.MouseDown[1] = true;
                break;
            case WM_RBUTTONUP:
                io.MouseDown[1] = false;
                break;
            case WM_MBUTTONDOWN:
                io.MouseDown[2] = true;
                break;
            case WM_MBUTTONUP:
                io.MouseDown[2] = false;
                break;
            case WM_MOUSEWHEEL:
                io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
                break;
            case WM_MOUSEMOVE:
                io.MousePos.x = (signed short)(lParam);
                io.MousePos.y = (signed short)(lParam >> 16);
                break;
            case WM_KEYDOWN:
                if (wParam < 256)
                    io.KeysDown[wParam] = 1;
                break;
            case WM_KEYUP:
                if (wParam < 256)
                    io.KeysDown[wParam] = 0;
                break;
            case WM_CHAR:
                if (wParam > 0 && wParam < 0x10000)
                    io.AddInputCharacter((unsigned short)wParam);
                break;
        }
        if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput) {
            return true;
        }
    }
        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);

}

namespace Hooks {
    void Init( ) {
        g_hWindow = U::GetProcessWindow( );

#ifdef DISABLE_LOGGING_CONSOLE
        bool bNoConsole = GetConsoleWindow( ) == NULL;
        if (bNoConsole) {
            AllocConsole( );
        }
#endif

        RenderingBackend_t eRenderingBackend = U::GetRenderingBackend( );
        switch (eRenderingBackend) {
            case DIRECTX9:
                DX9::Hook(g_hWindow);
                break;
            case DIRECTX10:
                DX10::Hook(g_hWindow);
                break;
            case DIRECTX11:
                DX11::Hook(g_hWindow);
                break;
            case DIRECTX12:
                DX12::Hook(g_hWindow);
                break;
            case OPENGL:
                GL::Hook(g_hWindow);
                break;
            case VULKAN:
                VK::Hook(g_hWindow);
                break;
        }

#ifdef DISABLE_LOGGING_CONSOLE
        if (bNoConsole) {
            FreeConsole( );
        }
#endif

        oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    }

    void Free( ) {
        if (oWndProc) {
            SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
        }

        MH_DisableHook(MH_ALL_HOOKS);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        RenderingBackend_t eRenderingBackend = U::GetRenderingBackend( );
        switch (eRenderingBackend) {
            case DIRECTX9:
                DX9::Unhook( );
                break;
            case DIRECTX10:
                DX10::Unhook( );
                break;
            case DIRECTX11:
                DX11::Unhook( );
                break;
            case DIRECTX12:
                DX12::Unhook( );
                break;
            case OPENGL:
                GL::Unhook( );
                break;
            case VULKAN:
                VK::Unhook( );
                break;
        }
    }
} // namespace Hooks
