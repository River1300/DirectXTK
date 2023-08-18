#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

class Game final : public DX::IDeviceNotify	// DeviceResources에 Game을 등록
{	//		=> DeviceResources에서 선언한 인터페이스 IDeviceNotify를 구현하여 장치가 변경되었을 때 Game 클래스가 대응할 수 있게
//				=> Game 클래스는 인터페이스가 가진 OnDeviceLost(), OnDeviceRestored() 함수를 반드시 구현해 줘야 한다.
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
	void OnDeviceRestored() override;	// 가상 함수에 연결

	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowMoved();
	void OnWindowSizeChanged(int width, int height);

	void GetDefaultSize(int& width, int& height) const noexcept;

private:
	void Update(DX::StepTimer const& timer);	// Update는 100% 시간 기반으러 만들어야 한다.
	void Render();	// CPU가 처리하는 일과 GPU가 처리하는 일을 구분

	void Clear();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

private:
	std::unique_ptr<DX::DeviceResources>	m_deviceResources;	// Com 인터페이스가 아닌 직접 만는 클래스 이므로 스마트 포인터로 생성
	DX::StepTimer							m_timer;

	std::unique_ptr<DirectX::Keyboard>		m_keyboard;
	std::unique_ptr<DirectX::Mouse>			m_mouse;
};

