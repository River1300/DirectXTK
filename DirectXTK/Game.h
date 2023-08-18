#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

class Game final : public DX::IDeviceNotify	// DeviceResources�� Game�� ���
{	//		=> DeviceResources���� ������ �������̽� IDeviceNotify�� �����Ͽ� ��ġ�� ����Ǿ��� �� Game Ŭ������ ������ �� �ְ�
//				=> Game Ŭ������ �������̽��� ���� OnDeviceLost(), OnDeviceRestored() �Լ��� �ݵ�� ������ ��� �Ѵ�.
public:
	Game() noexcept(false);
	~Game();

	Game(Game&&) = default;
	Game& operator=(Game&&) = default;

	Game(Game const&) = delete;
	Game& operator=(Game const&) = delete;

public:
	void Initialize(HWND window, int width, int height);
	void Tick();

	void OnDeviceLost() override;
	void OnDeviceRestored() override;	// ���� �Լ��� ����

	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowMoved();
	void OnWindowSizeChanged(int width, int height);

	void GetDefaultSize(int& width, int& height) const noexcept;

private:
	void Update(DX::StepTimer const& timer);	// Update�� 100% �ð� ������� ������ �Ѵ�.
	void Render();	// CPU�� ó���ϴ� �ϰ� GPU�� ó���ϴ� ���� ����

	void Clear();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

private:
	std::unique_ptr<DX::DeviceResources>	m_deviceResources;	// Com �������̽��� �ƴ� ���� ���� Ŭ���� �̹Ƿ� ����Ʈ �����ͷ� ����
	DX::StepTimer							m_timer;

	std::unique_ptr<DirectX::Keyboard>		m_keyboard;
	std::unique_ptr<DirectX::Mouse>			m_mouse;
};

