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

void ExitGame() noexcept
{
	PostQuitMessage(0);
}
