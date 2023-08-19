#include "pch.h"
#include "Game.h"

using namespace DirectX;

namespace
{
	std::unique_ptr<Game> g_game;
}	// �������� ���� �ν��Ͻ��� unique_ptr�� �����ϰ� �ܺο����� ����� �Ұ����ϵ��� ���� �̸� ���忡 ��� �д�.

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// C �� C++ �� �Լ��� ������/��ũ�� �� �ٸ� ���¸� ����Ѵ�. C �� �Լ��� �״�� ����� ����������, C++ �� ���������� ���� �Լ� �տ�
// �߰� ������ �ٰ� �ȴ�. extern "C" �� ����ϸ� �ش� ���� C ���� �������� ������ ����.
// ������ Ÿ���� �ƴ� ���� �ð��� �����ϴ� DLL ���̺귯������ ȣȯ���� �����ϱ� ���� C ���� �ۼ��Ǵ� ��찡 ����.
extern "C"
{
	// ��ǻ���� �׷��� ���ó�� ��ȯ�Ͽ� �뵵�� �´� �׷��� ����͸� ����ϵ��� �ϴ� ����
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// #. ��Ŀ ����
//		=> ���ݱ����� ������Ͽ� #pragma Ű���带 ����� ���̺귯�� ������ ��ũ�ߴ�.
//	1. ������Ʈ�� -> �Ӽ�
//	2. ��� ���� + ��� �÷���
//	3. ���� �Ӽ� -> ��Ŀ -> �Է� �׸�
//	4. �߰� ���Ӽ� -> ����
//	5. �߰� ���Ӽ� .lib�� �߰�

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!XMVerifyCPUSupport()) return 1;	// DirectXMath ���̺귯���� �ش� �÷����� �����ϴ����� ��ȯ

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);	// WIC�� ������ �� ����� COM ȯ�� �ʱ�ȭ ~ ����
	//	=> COM�� ��Ƽ ������ ���� �ʱ�ȭ
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
	static bool s_fullscreen = false;	// �������� ���¸� �����ϱ� ���� ���� �÷��׵�

	// �����쿡�� ����� �����͸� ������ Game �ν��Ͻ��� ��ȯ
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
		if (wParam == SIZE_MINIMIZED)	// �ּ�ȭ �޼����� �޾��� ��
		{
			if (!s_minimized)	// �ּ�ȭ ���� ���� ���¶��
			{
				s_minimized = true;	// �ּ�ȭ �÷���
				if (!s_in_suspend && game) game->OnSuspending();
				s_in_suspend = true;	// �Ͻ� ���� �÷���
			}
		}
		else if (s_minimized)	// �ּ�ȭ �޽����� �ƴϰ� �̹� �ּ�ȭ �Ǿ� ������
		{
			s_minimized = false;	// �ּ�ȭ �÷���
			if (s_in_suspend && game) game->OnResuming();
			s_in_suspend = false;	// �Ͻ� ���� �÷���
		}
		else if (!s_in_sizemove && game)	// ������ ���� ���� �ƴ� ��
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
		case PBT_APMQUERYSUSPEND:	// �������� ���� ���� �˸�
			if (!s_in_suspend && game) game->OnSuspending();
			s_in_suspend = true;
			return TRUE;

		case PBT_APMRESUMESUSPEND:	// ������忡�� �簳�ʿ� ���� �˸�
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
			if (s_fullscreen)	// ���� ���°� ��üȭ��
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);	// ���ƿ� â����� ������ ��Ÿ�� ����

				int width = 800;
				int height = 600;
				if (game) game->GetDefaultSize(width, height);	// �⺻ ������ ũ�⸦ ��������

				ShowWindow(hWnd, SW_SHOWNORMAL);

				SetWindowPos(	// ������ ��ġ ����
					hWnd,
					HWND_TOP,
					0, 0, width, height,
					SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED
				);
			}
			else	// ������ �����̸�
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, 0);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);	// ��üȭ��� ������ ��Ÿ�� ����

				SetWindowPos(	// ������ ��ġ ����
					hWnd,
					HWND_TOP,
					0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

				ShowWindow(hWnd, SW_SHOWMAXIMIZED);	// �ִ�ȭ
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
