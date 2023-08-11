#pragma once

// #. Device Lost ��ó
//		=> ������ ũ�Ⱑ ����Ǿ��� �� �ٽ� �����ؾ� �ϴ� �ػ󵵿� ���õ� ���ҽ�
//			=> SwapChain( ����� )
//			=> Depth ~ Stencil Buffer
//		=> ����̽��� �սǵǾ��� �� �ٽ� �����ؾ� �ϴ� ���ҽ�
//	D3DDevice, Device, Dependent Resource( �ؽ���, ����������, ���̴� �� ), �ػ� ���� ���ҽ��� ���� �ٽ� ����� �ش�.

// #. ����ڰ� ���� �� Alt + Tab�� ������ �׷��� ����̹��� ������Ʈ�Ѵٸ�
//		=> Device�� �ջ�� ������ ������ �Ѵ�. -> ������ �����ϸ� Device Dependent Resource�� �ҷ��ش�.

namespace DX
{
// #. Interface : ���� ���� �Լ��� �̷���� Ŭ����
//		=> Device�� ���õ� ����� �߰��� �� ���Ѿ��� �Ծ��� ���� ���´�.
//			=> ����̽� �ս�, ����̽� ������ ���� �Ծ�
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;		// ����̽��� �սǵǾ��� ���� ó���� ���� ���� ���� �Լ�
		virtual void OnDeivecRestored() = 0;	// ����̽��� �����Ǿ��� ���� ó���� ���� ���� ���� �Լ�

	protected:
		~IDeviceNotify() = default;	// �������� ������ �ִ� ����ü�� ���������� POD���·� ��ȯ�ϱ� ���� �Ҹ��ڸ� protected�� ����� default�� �⺻ �����ڷ� �����.
	//		=> �ش� ��ü�� ���������� �޸� ������ ���� �ʴ´ٴ� ��
	};

	class DeviceResources
	{
	public:
// #. ��ȣ ���� ��Ʈ �÷���
		static const unsigned int c_FlipPresent = 0x1;		// �ø� ȿ�� �÷���
		static const unsigned int c_AllowTearing = 0x2;		// �׾ ��� �÷���
		static const unsigned int c_EnableHDR = 0x4;		// HDR ��� �÷���

		DeviceResources(
			DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
			UINT backBufferCount = 2,
			D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_10_0,
			unsigned int flags = c_FlipPresent
		) noexcept;
		// �� Ŭ���������� ���ο��� ���ҽ��� �߰������� �����Ҵ� ���� �ʴ´�.
		~DeviceResources() = default;

		DeviceResources(DeviceResources&&) = default;				// �̵� �����ڴ� �����Ϸ��� �ڵ����� ����
		DeviceResources& operator= (DeviceResources&&) = default;	// �̵� ���� �����ڴ� �����Ϸ��� �ڵ����� ����
		//		=> �̵� �����ڰ� �⺻��( default ) : �ٸ� �ν��Ͻ��� ����Ͽ� ���� �ν��Ͻ��� ���� ��( �̵��� �� : �޸� �������� ��� �̵� ) 

		DeviceResources(DeviceResources const&) = delete;				// ���� �����ڴ� ����, �ν��Ͻ��� �ٸ� ������ ������ �� ���簡 �Ͼ�� �ʴ´�.
		DeviceResources& operator= (DeviceResources const&) = delete;	// ���� ���� �����ڸ� ����
		//		=> ����̽� ���� : �������� ����̽��� �����Ǵ� ���� �����ϱ� ���� ����

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
// #. Annotation : �ּ�
//		=> Comment : �ҽ� �ڵ忡 �ٴ� �ּ�
//		=> Annotation�� �׷��� ī�� ��ɿ� �ٴ� �ּ�
//		=> ��, ������ ������ �ܰ迡 �ּ��� �ް� ������ ����� CPU�� ���޵Ǿ� ������� ���� ���� �� �� �ְ� ���ش�.
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
// #. PIX : �׷��Ƚ� ���� ����� ���
//	����� -> �׷��� -> �׷��� �����
// �Լ��� wrapper ������ �Լ���� ������ ǥ���ϴ� PIXBeginfEvent() ~ PIXEndEvent(), ��Ŀ�� ǥ���ϴ� PIXSetMrker() �Լ��� �Ǿ� �ִ�.
		void PIXBeginEvent(_In_z_ const wchar_t* name) { m_d3dAnnotation->BeginEvent(name); }
		void PIXEndEvent() { m_d3dAnnotation->EndEvent(); }
		void PIXSetMarker(_In_z_ const wchar_t* name) { m_d3dAnnotation->SetMarker(name); }
	};
}
