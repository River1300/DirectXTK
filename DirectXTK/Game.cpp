#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;
//		=> 외부에 존재하는 ExitGame() 함수에 대한 extern 선언
//		=> 운영체제 별로 서로 다른 종료 스타일이기 때문에 Game 클래스 외부에 종료 함수를 만든다.

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
	m_deviceResources = std::make_unique<DX::DeviceResources>();
	m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
}

void Game::Initialize(HWND window, int width, int height)
{
	m_keyboard = std::make_unique<Keyboard>();
	m_mouse = std::make_unique<Mouse>();	// 키보드와 마우스를 처리하기 위한 인스턴스
	m_mouse->SetWindow(window);	// 윈도우 핸들 지정

	m_deviceResources->SetWindow(window, width, height);	// 디바이스 리소스에 윈도우와 크기 지정

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();		// 디바이스 종속 리소스 생성

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();	// 윈도우 크기에 종속 리소스 생성
}

// "Frame Update"라는 지역을 선언하여 코드 접기 및 펼치기 기능을 지원
#pragma region Frame Update
void Game::Tick()
{
	m_timer.Tick([&]()
		{
			Update(m_timer);
		});	//	=> 게임의 Tick 신호를 처리, StepTimer::Update() 함수는 내부적으로 고정 프레임을 지원하며 고정 프레임 시간이 될 때까지 내부에서 매개 변수로 받은 업데이트 함수를 반복한다.
	Render();
}
void Game::Update(DX::StepTimer const& timer)	// 게임 로직을 업데이트 하는 함수
{
	// GAME LOOP

	auto kb = m_keyboard->GetState();
	if (kb.Escape)
	{
		ExitGame();
	}
}
#pragma endregion

#pragma region Frame Render
void Game::Render()
{
	// 최소한 한 번의 업데이트가 보장되도록 만듬
	if (m_timer.GetFrameCount() == 0) return;

	Clear();

	m_deviceResources->PIXBeginEvent(L"Render");

	m_deviceResources->PIXEndEvent();

	m_deviceResources->Present();	// 모니터에 출력
}
void Game::Clear()
{
	m_deviceResources->PIXBeginEvent(L"Clear");

	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTarget = m_deviceResources->GetRenderTargetView();
	auto depthStencil = m_deviceResources->GetDepthStencilView();

	context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, &renderTarget, depthStencil);

	auto viewport = m_deviceResources->GetScreenVieport();
	context->RSSetViewports(1, &viewport);

	m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handles
void Game::OnActivated()
{
	// 윈도우가 활성화 될 때 수행할 작업을 작성
}
void Game::OnDeactivated()
{
	// 윈도우가 비활성화 될 때 수행할 작업을 작성
}
void Game::OnSuspending()
{
	// 절전모드 등의 상태로 일시 정지 상태로 전활될 때 수행할 작업을 작성
}
void Game::OnResuming()
{
	m_timer.ResetElapsedTime();	// 일시 정지 상태에서 재개될 때 수행할 작업을 작성
}
void Game::OnWindowMoved()	// 윈도우가 이동되었을 때 수행할 작업을 작성
{
	auto r = m_deviceResources->GetOutputSize();
	m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
void Game::OnWindowSizeChanged(int width, int height)	// 윈도우 크기가 변경되었을 때의 처리
{
	if (!m_deviceResources->WindowSizeChanged(width, height)) return;

	CreateWindowSizeDependentResources();
}
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
	width = 800;
	height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
void Game::CreateDeviceDependentResources()
{
	// 텍스쳐, 버텍스 등 디바이스 종속 리소스를 생성
}
void Game::CreateWindowSizeDependentResources()
{
	// 프로젝션 등 윈도우 크기 종속 리소스를 생성
}
void Game::OnDeviceLost()
{
	// 디바이스가 손실 되었을 때의 처리
}
void Game::OnDeviceRestored()	// 디바이스가 복구 되었을 때의 처리
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
#pragma endregion
