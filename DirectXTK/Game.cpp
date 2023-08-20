#include "pch.h"
#include "Game.h"

#include <iomanip>
#include <sstream>

extern void ExitGame() noexcept;
//		=> �ܺο� �����ϴ� ExitGame() �Լ��� ���� extern ����
//		=> �ü�� ���� ���� �ٸ� ���� ��Ÿ���̱� ������ Game Ŭ���� �ܺο� ���� �Լ��� �����.

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
	m_mouse = std::make_unique<Mouse>();	// Ű����� ���콺�� ó���ϱ� ���� �ν��Ͻ�
	m_mouse->SetWindow(window);	// ������ �ڵ� ����

	m_deviceResources->SetWindow(window, width, height);	// ����̽� ���ҽ��� ������� ũ�� ����

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();		// ����̽� ���� ���ҽ� ����

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();	// ������ ũ�⿡ ���� ���ҽ� ����
}

// "Frame Update"��� ������ �����Ͽ� �ڵ� ���� �� ��ġ�� ����� ����
#pragma region Frame Update
void Game::Tick()
{
	m_timer.Tick([&]()
		{
			Update(m_timer);
		});	//	=> ������ Tick ��ȣ�� ó��, StepTimer::Update() �Լ��� ���������� ���� �������� �����ϸ� ���� ������ �ð��� �� ������ ���ο��� �Ű� ������ ���� ������Ʈ �Լ��� �ݺ��Ѵ�.
	Render();
}
void Game::Update(DX::StepTimer const& timer)	// ���� ������ ������Ʈ �ϴ� �Լ�
{
	// GAME LOOP

	auto kb = m_keyboard->GetState();
	if (kb.Escape)
	{
		ExitGame();
	}
	// ���� �ִϸ��̼� ������ ó��
	m_timeToNextFrame -= timer.GetElapsedSeconds();	// ���� �ð����� ����ð�(��Ÿ)�� ����.
	if (m_timeToNextFrame < 0.f)
	{
		m_timeToNextFrame = 0.1f;	// ���� �ð��� 0.1�ʷ� �����ϰ�
		m_currentFrame = (m_currentFrame + 1) %	// �ִϸ��̼� �������� ��ȯ������ ���� 0 ~ 9, 0 ~ 9 ...
			static_cast<int>(m_textures.size());
	}
}
#pragma endregion

#pragma region Frame Render
void Game::Render()
{
	// �ּ��� �� ���� ������Ʈ�� ����ǵ��� ����
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

	m_deviceResources->Present();	// ����Ϳ� ���
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
	// �����찡 Ȱ��ȭ �� �� ������ �۾��� �ۼ�
}
void Game::OnDeactivated()
{
	// �����찡 ��Ȱ��ȭ �� �� ������ �۾��� �ۼ�
}
void Game::OnSuspending()
{
	// ������� ���� ���·� �Ͻ� ���� ���·� ��Ȱ�� �� ������ �۾��� �ۼ�
}
void Game::OnResuming()
{
	m_timer.ResetElapsedTime();	// �Ͻ� ���� ���¿��� �簳�� �� ������ �۾��� �ۼ�
}
void Game::OnWindowMoved()	// �����찡 �̵��Ǿ��� �� ������ �۾��� �ۼ�
{
	auto r = m_deviceResources->GetOutputSize();
	m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
void Game::OnWindowSizeChanged(int width, int height)	// ������ ũ�Ⱑ ����Ǿ��� ���� ó��
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

	// �ؽ���, ���ؽ� �� ����̽� ���� ���ҽ��� ����
	std::wstringstream fileName;
	for (int i = 0; i < 10; i++)
	{
		fileName.str(L"");
		fileName << L"Assets/die" << std::setfill(L'0') << std::setw(2) << i + 1 << ".png";	// ���� �� ����
		DX::ThrowIfFailed(
			CreateWICTextureFromFile(	// WIC �� ����Ͽ� ���Ͽ��� �ؽ��ĸ� ����
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
	// �������� �� ������ ũ�� ���� ���ҽ��� ����
}
void Game::OnDeviceLost()
{
	for (auto tex : m_textures) tex.Reset();

	m_spriteBatch.reset();
	m_commonStates.reset();
}
void Game::OnDeviceRestored()	// ����̽��� ���� �Ǿ��� ���� ó��
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
#pragma endregion
