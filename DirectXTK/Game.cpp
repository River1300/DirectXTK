#include "pch.h"
#include "Game.h"

#include <iomanip>
#include <sstream>

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
	// 다음 애니메이션 프레임 처리
	m_timeToNextFrame -= timer.GetElapsedSeconds();	// 남은 시간에서 경과시간(델타)를 뺀다.
	if (m_timeToNextFrame < 0.f)
	{
		m_timeToNextFrame = 0.1f;	// 다음 시간을 0.1초로 지정하고
		m_currentFrame = (m_currentFrame + 1) %	// 애니메이션 프레임을 순환형으로 증가 0 ~ 9, 0 ~ 9 ...
			static_cast<int>(m_textures.size());
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

	m_spriteBatch->Begin(
		SpriteSortMode_Deferred, m_commonStates->NonPremultiplied()
	);
	m_spriteBatch->Draw(
		m_textures[m_currentFrame].Get(),
		XMFLOAT2(0, 0)
	);
	m_spriteBatch->End();

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
	auto device = m_deviceResources->GetD3DDevice();
	auto context = m_deviceResources->GetD3DDeviceContext();

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

	// 텍스쳐, 버텍스 등 디바이스 종속 리소스를 생성
	std::wstringstream fileName;
	for (int i = 0; i < 10; i++)
	{
		fileName.str(L"");
		fileName << L"Assets/die" << std::setfill(L'0') << std::setw(2) << i + 1 << ".png";	// 파일 명 생성
		DX::ThrowIfFailed(
			CreateWICTextureFromFile(	// WIC 를 사용하여 파일에서 텍스쳐를 생성
				device,
				fileName.str().c_str(),
				nullptr,
				m_textures[i].ReleaseAndGetAddressOf()
			)
		);
	}
}
void Game::CreateWindowSizeDependentResources()
{
	// 프로젝션 등 윈도우 크기 종속 리소스를 생성
}
void Game::OnDeviceLost()
{
	for (auto tex : m_textures) tex.Reset();

	m_spriteBatch.reset();
	m_commonStates.reset();
}
void Game::OnDeviceRestored()	// 디바이스가 복구 되었을 때의 처리
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
#pragma endregion
