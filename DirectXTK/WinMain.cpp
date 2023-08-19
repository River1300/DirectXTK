#include "pch.h"
#include "Game.h"

using namespace DirectX;

namespace
{
	std::unique_ptr<Game> g_game;
}	// 전역으로 게임 인스턴스를 unique_ptr로 생성하고 외부에서는 사용이 불가능하도록 무명 이름 공장에 담아 둔다.

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// C 와 C++ 은 함수를 컴파일/링크할 때 다른 형태를 사용한다. C 의 함수는 그대로 사용이 가능하지만, C++ 은 다형성으로 인해 함수 앞에
// 추가 정보가 붙게 된다. extern "C" 를 사용하면 해당 블럭을 C 언어로 컴피일이 가능해 진다.
// 컴파일 타임이 아닌 동적 시간에 참조하는 DLL 라이브러리들은 호환성을 유지하기 위해 C 언어로 작성되는 경우가 많다.
extern "C"
{
	// 컴퓨터의 그래픽 어댑처를 전환하여 용도에 맞는 그래픽 어댑터를 사용하도록 하는 개념
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// #. 링커 파일
//		=> 지금까지는 헤더파일에 #pragma 키워드를 사용해 라이브러리 파일을 링크했다.
//	1. 프로젝트명 -> 속성
//	2. 모든 구성 + 모든 플랫폼
//	3. 구성 속성 -> 링커 -> 입력 항목
//	4. 추가 종속성 -> 편집
//	5. 추가 종속성 .lib를 추가

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!XMVerifyCPUSupport()) return 1;	// DirectXMath 라이브러리가 해당 플랫폼을 지원하는지를 반환

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);	// WIC를 적용할 때 사용한 COM 환경 초기화 ~ 해제
	//	=> COM을 멀티 쓰레드 모드로 초기화
	if (FAILED(hr)) return 1;

	g_game = std::make_unique<Game>();
	{
		WNDCLASSEXW wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEXW);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
		wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wcex.lpszClassName = L"DirectXTKSimpleSampleWindowClass";
		wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");

		if (!RegisterClassExW(&wcex)) return 1;

		int w, h;
		g_game->GetDefaultSize(w, h);

		RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		HWND hwnd = CreateWindowExW(
			0,
			L"DirectXTKSimpleSampleWindowClass",
			L"DirectXTKSimpleSample",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rc.right - rc.left,
			rc.bottom - rc.top,
			nullptr,
			nullptr,
			hInstance,
			nullptr
		);

		if (!hwnd) return 1;

		ShowWindow(hwnd, nCmdShow);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

		GetClientRect(hwnd, &rc);

		g_game->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top);
	}

	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			g_game->Tick();
		}
	}
	
	g_game.reset();

	CoUninitialize();

	return static_cast<int>(msg.wParam);
}

LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;	// 윈도우의 상태를 추적하기 위한 정적 플래그들

	// 윈도우에서 사용자 데이터를 가져와 Game 인스턴스로 변환
	auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_PAINT:
		if (s_in_sizemove && game)
		{
			game->Tick();
		}
		else
		{
			PAINTSTRUCT ps;
			(void)BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_MOVE:
		if (game)
		{
			game->OnWindowMoved();
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)	// 최소화 메세지를 받았을 때
		{
			if (!s_minimized)	// 최소화 되지 않은 상태라면
			{
				s_minimized = true;	// 최소화 플래그
				if (!s_in_suspend && game) game->OnSuspending();
				s_in_suspend = true;	// 일시 정지 플래그
			}
		}
		else if (s_minimized)	// 최소화 메시지가 아니고 이미 최소화 되어 있으면
		{
			s_minimized = false;	// 최소화 플래그
			if (s_in_suspend && game) game->OnResuming();
			s_in_suspend = false;	// 일시 정지 플래그
		}
		else if (!s_in_sizemove && game)	// 사이즈 변경 중이 아닐 때
		{
			game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
		}
		break;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (game)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
		}
		break;

	case WM_GETMINMAXINFO:
		if (lParam)
		{
			auto info = reinterpret_cast<MINMAXINFO*>(lParam);
			info->ptMinTrackSize.x = 320;
			info->ptMinTrackSize.y = 200;
		}
		break;

	case WM_ACTIVATEAPP:
		if (game)
		{
			if (wParam)
			{
				game->OnActivated();
			}
			else
			{
				game->OnDeactivated();
			}
		}
		break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:	// 절전모드로 들어감에 대한 알림
			if (!s_in_suspend && game) game->OnSuspending();
			s_in_suspend = true;
			return TRUE;

		case PBT_APMRESUMESUSPEND:	// 절전모드에서 재개됨에 대한 알림
			if (!s_minimized)
			{
				if (s_in_suspend && game) game->OnResuming();
				s_in_suspend = false;
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse::ProcessMessage(message, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard::ProcessMessage(message, wParam, lParam);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)	// ALT + ENTER
		{
			if (s_fullscreen)	// 현재 상태가 전체화면
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);	// 돌아올 창모드의 윈도우 스타일 지정

				int width = 800;
				int height = 600;
				if (game) game->GetDefaultSize(width, height);	// 기본 위도우 크기를 가져오고

				ShowWindow(hWnd, SW_SHOWNORMAL);

				SetWindowPos(	// 윈도우 위치 지정
					hWnd,
					HWND_TOP,
					0, 0, width, height,
					SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED
				);
			}
			else	// 윈도우 상태이면
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, 0);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);	// 전체화면용 윈도우 스타일 지정

				SetWindowPos(	// 윈도우 위치 지정
					hWnd,
					HWND_TOP,
					0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

				ShowWindow(hWnd, SW_SHOWMAXIMIZED);	// 최대화
			}
			s_fullscreen = !s_fullscreen;
		}
		Keyboard::ProcessMessage(message, wParam, lParam);
		break;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void ExitGame() noexcept
{
	PostQuitMessage(0);
}
