#pragma once

// #. Device Lost 대처
//		=> 윈도우 크기가 변경되었을 때 다시 생성해야 하는 해상도와 관련된 리소스
//			=> SwapChain( 백버퍼 )
//			=> Depth ~ Stencil Buffer
//		=> 디바이스가 손실되었을 때 다시 생성해야 하는 리소스
//	D3DDevice, Device, Dependent Resource( 텍스쳐, 파이프라인, 셰이더 등 ), 해상도 관련 리소스를 전부 다시 만들어 준다.

// #. 사용자가 게임 중 Alt + Tab을 누르고 그래픽 드라이버를 업데이트한다면
//		=> Device가 손상된 시점에 복구를 한다. -> 복구를 성공하면 Device Dependent Resource를 불러준다.

namespace DX
{
// #. Interface : 순수 가상 함수로 이루어진 클래스
//		=> Device와 관련된 기능을 추가할 때 지켜야할 규약을 정해 놓는다.
//			=> 디바이스 손실, 디바이스 복구에 대한 규약
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;		// 디바이스가 손실되었을 때의 처리를 위한 순수 가상 함수
		virtual void OnDeivecRestored() = 0;	// 디바이스가 복원되었을 때의 처리를 위한 순수 가상 함수

	protected:
		~IDeviceNotify() = default;	// 다형성을 가지고 있는 구조체를 강제적으로 POD형태로 전환하기 위해 소멸자를 protected로 숨기고 default로 기본 생성자로 만든다.
	//		=> 해당 객체는 내부적으로 메모리 관리를 하지 않는다는 뜻
	};

	class DeviceResources
	{
	public:
// #. 부호 없는 비트 플래그
		static const unsigned int c_FlipPresent = 0x1;		// 플립 효과 플래그
		static const unsigned int c_AllowTearing = 0x2;		// 테어링 허용 플래그
		static const unsigned int c_EnableHDR = 0x4;		// HDR 사용 플래그

		DeviceResources(
			DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
			UINT backBufferCount = 2,
			D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_10_0,
			unsigned int flags = c_FlipPresent
		) noexcept;
		// 이 클래스에서는 내부에서 리소스를 추가적으로 동적할당 하지 않는다.
		~DeviceResources() = default;

		DeviceResources(DeviceResources&&) = default;				// 이동 생성자는 컴파일러가 자동으로 생성
		DeviceResources& operator= (DeviceResources&&) = default;	// 이동 대입 연산자는 컴파일러가 자동으로 생성
		//		=> 이동 생성자가 기본값( default ) : 다른 인스턴스를 사용하여 현재 인스턴스를 만들 때( 이동할 때 : 메모리 소유권이 모두 이동 ) 

		DeviceResources(DeviceResources const&) = delete;				// 복사 생성자는 제거, 인스턴스를 다른 곳으로 대입할 때 복사가 일어나지 않는다.
		DeviceResources& operator= (DeviceResources const&) = delete;	// 복사 대입 연산자를 제거
		//		=> 디바이스 관리 : 여러개의 디바이스가 생성되는 것을 방지하기 위해 제거

	public:
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void SetWindow(HWND window, int width, int height) noexcept;
		bool WindowSizeChanged(int width, int height);
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept
		{
			m_deviceNotify = deviceNotify;
		}
		void Present();

	private:
		void CreateFactory();
		void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
		void UpdateColorSpace();

		Microsoft::WRL::ComPtr<IDXGIFactory2>				m_dxgiFactory;
		Microsoft::WRL::ComPtr<ID3D11Device1>				m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext1>		m_d3dContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain1>				m_swapChain;
// #. Annotation : 주석
//		=> Comment : 소스 코드에 다는 주석
//		=> Annotation은 그래픽 카드 기능에 다는 주석
//		=> 즉, 랜더링 파이프 단계에 주석을 달고 문제가 생기면 CPU로 전달되어 디버깅을 비교적 쉽게 할 수 있게 해준다.
		Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>	m_d3dAnnotation;

		Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_renderTarget;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_depthStencil;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		m_d3dRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		m_d3dDepthStencilView;
		D3D11_VIEWPORT										m_screenViewport;
		DXGI_FORMAT											m_backBufferFormat;
		DXGI_FORMAT											m_depthBufferFormat;
		UINT												m_backBufferCount;
		D3D_FEATURE_LEVEL									m_d3dMinFeatureLevel;

		HWND												m_window;
		D3D_FEATURE_LEVEL									m_d3dFeatureLevel;
		RECT												m_outputSize;

		DXGI_COLOR_SPACE_TYPE								m_colorSpace;

		unsigned int										m_options;

		IDeviceNotify* m_deviceNotify;

	public:
		RECT GetOutputSize() const noexcept { return m_outputSize; }

		auto GetD3DDevice() const noexcept				{ return m_d3dDevice.Get(); }
		auto GetD3DDeviceContext() const noexcept	{ return m_d3dContext.Get(); }
		auto GetSwapChain() const noexcept				{ return m_swapChain.Get(); }
		auto GetDXGIFactory() const noexcept				{ return m_dxgiFactory.Get(); }

		auto GetWindow() const noexcept								{ return m_window; }

		auto GetDeviceFeatureLevel() const noexcept		{ return m_d3dFeatureLevel; }
		auto GetRenderTarget() const noexcept			{ return m_renderTarget.Get(); }
		auto GetDepthStencil() const noexcept			{ return m_depthStencil.Get(); }
		auto GetRenderTargetView() const noexcept	{ return m_d3dRenderTargetView.Get(); }
		auto GetDepthStencilView() const noexcept	{ return m_d3dDepthStencilView.Get(); }

		auto GetBackBufferFormat() const noexcept				{ return m_backBufferFormat; }
		auto GetDepthBufferFormat() const noexcept			{ return m_depthBufferFormat; }
		auto GetScreenVieport() const noexcept				{ return m_screenViewport; }
		auto GetBackBufferCount() const noexcept					{ return m_backBufferCount; }

		auto GetColorSpace() const noexcept			{ return m_colorSpace; }
		auto GetDeviceOptions() const noexcept				{ return m_options; }

	public:
// #. PIX : 그래픽스 관련 디버깅 기능
//	디버그 -> 그래픽 -> 그래픽 디버깅
// 함수는 wrapper 수준의 함수들로 영역을 표시하는 PIXBeginfEvent() ~ PIXEndEvent(), 마커를 표기하는 PIXSetMrker() 함수로 되어 있다.
		void PIXBeginEvent(_In_z_ const wchar_t* name) { m_d3dAnnotation->BeginEvent(name); }
		void PIXEndEvent() { m_d3dAnnotation->EndEvent(); }
		void PIXSetMarker(_In_z_ const wchar_t* name) { m_d3dAnnotation->SetMarker(name); }
	};
}
