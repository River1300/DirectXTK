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

void ExitGame() noexcept
{
	PostQuitMessage(0);
}
