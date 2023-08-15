#include "pch.h"	// ���� ������ ����� ������ �ϸ� .cpp�� ���� �� �ڵ����� �߰��ȴ�.
#include "DeviceResources.h"

using namespace DirectX;
using namespace DX;
using Microsoft::WRL::ComPtr;

// #. clang( Ŭ�� ): C, C++, Object-C �� ���� �����Ϸ�
//      => �����Ϸ� ����Ʈ���� : �鿣��� LLVM�̶� �����Ϸ��� ����Ѵ�.
//      => ���¼ҽ��� GNU Compiler�� ��ü�ϱ� ���� ���������� ���н� �迭���� �ַ� ����ϴ� �����Ϸ�
//      => __clang__�� ����Ǿ� �ִٸ� ifedf ~ endif �ȿ� �ִ� ����� �����ϰ� �ƴϸ� ����
#ifdef __clang__
#pragma clang diagnostic ignored "_Wcovered-switch-detaulf" // ������ �ü���� �ƴ� �ٸ� �ü������ ����� �� �ִ� C, C++...
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace  // �̸� ���� ���ӽ����̽� : �� ���������� ����� �� �ִ� ����
{
#if defined(_DEBUG) // Debug ���� ������ ��
    inline bool SdkLayerAvailable() noexcept
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,
            nullptr,
            D3D11_CREATE_DEVICE_DEBUG,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            nullptr,
            nullptr,
            nullptr
        );
        return SUCCEEDED(hr);   // �ζ��� �Լ��� ����׸��� ��� ������ DirectX ����̽��� �����ϴ��� üũ
    }
#endif

    // �̸���� DXGI_FORMAT�� SRGB��尡 �ƴ� �������� �����ϴ� �Լ�
    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) noexcept
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        default:
            return fmt;
        }
    }
}

DeviceResources::DeviceResources(
    DXGI_FORMAT backBufferFormat, 
    DXGI_FORMAT depthBufferFormat, 
    UINT backBufferCount, 
    D3D_FEATURE_LEVEL minFeatureLevel, 
    unsigned int flags
) noexcept : m_screenViewport{}, m_backBufferFormat(backBufferFormat), m_depthBufferFormat(depthBufferFormat),
    m_backBufferCount(backBufferCount), m_d3dMinFeatureLevel(minFeatureLevel), m_window(nullptr),
    m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1), m_outputSize{ 0,0,1,1 }, m_colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709),
    m_options(flags | c_FlipPresent), m_deviceNotify(nullptr)
{
}

void DeviceResources::CreateDeviceResources()   // ����� ��ǻ�� ȯ���� üũ�Ͽ� ������ ���� ȯ���� ����� �ش�.
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;  // ������� �׷���ī�尡 BGRA ���� ������ �����Ǵ��� Ȯ��

// #1. ����� üũ
#if defined(_DEBUG)
    if (SdkLayerAvailable())
    {
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
    else
    {
        OutputDebugStringA("WARNING: Direct3D Debug Device is not available \n");
    }
#endif

    CreateFactory();

// #2. �ɼǿ� ���� ��� ���� ���θ� üũ
    if (m_options & c_AllowTearing)
    {
        BOOL allowTearing = FALSE;  // �׾ ���� ����

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = m_dxgiFactory.As(&factory5);   // �������̽��� Factory5�� �����ϴ��� Ȯ��
        //  => �ϵ���� �׾ ��� �ɼ��� dxgi1_5.h���� �����Ǹ� IDXGFactory5 �������̽����� üũ�� �����ϴ�.
        //      => ���� dxgi �̳����̽��� IDXGIFactory5�� �������� ������ ������ ( as - a ) hr�� S_OK�� ��ȯ�ǰ�
        //          => Factory5�� m_dxgiFactory�� ��ȯ�Ͽ� ������ �ش�.

        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing,
                sizeof(allowTearing)
            );
        }
// �׾�� �׷��� �ջ�ó�� ���̴� ���� ���� ��������, �ֱ� ���� ���ļ� ������� �������� ������ �ٲ����. �������� V-Sync( ���� ����ȭ )
// �� �׷���ī�尡 ����͸� ��ٸ��� ����̾����� ���� ����͵��� ������ �׷���ī�带 ��ٸ��� ����� ����Ѵ�. ���� �׷��� ī��� �ִ���
// ���� ������Ʈ�� ����� �ϴµ� �̸� ���ؼ��� Tearing ��� �� ���� ����ȭ�� ����� �Ѵ�. ���� �̷��� �����ϴ� ��ŭ ����ʹ�
// �ִ��� ���� ���ļ��� ������ ���� ����. ( ���� 75Hz �̻��� ����͵��� ������ �̷� ����͸� �����Ϸ��� �� ����� üũ�� ��� �Ѵ�. )

        if (FAILED(hr) || !allowTearing)
        {
            m_options &= ~c_AllowTearing;
#ifdef _DEBUG
            OutputDebugStringA(
                "WARNING: Variable refresh rate display no supported"
            );
#endif
        }
    }

    if (m_options & c_EnableHDR)    // HDR ���� Ȯ��, ����̽��� �ƴ� ����͸� üũ�ϴ� ������ �ܼ��� DXGI ������ Ȯ��
    {
        ComPtr<IDXGIFactory5> factory5;
        if (FAILED(m_dxgiFactory.As(&factory5)))
        {
            m_options &= ~c_EnableHDR;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: HDR swap chains no supported");
#endif
        }
    }

    if (m_options & c_FlipPresent)  // �ø� ���������̼� ȿ�� ���� Ȯ��
    {
        ComPtr<IDXGIFactory4> factory4;
        if (FAILED(m_dxgiFactory.As(&factory4)))
        {
            m_options &= ~c_FlipPresent;
#ifdef _DEBUG
            OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
        }
    }

    // �����Ǵ� DirectX ������ Ȯ��
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    UINT featLevelCount = 0;
    for (; featLevelCount < _countof(s_featureLevels); featLevelCount++)
    {
        if (s_featureLevels[featLevelCount] < m_d3dMinFeatureLevel) break;
    }   // �Ѱ� ���� DX ������ �� �ɼ����� ������ Level���� ū �������� ������ ���Ѵ�.
    //      => �迭�� ū ���� ���� ������������ ���ĵǾ� �����Ƿ� ó�� Level���� ���� ���� ������ �ݺ����� �����Ѵ�.

    if (!featLevelCount)    // �ּ� �䱸������ �ʹ� ������ ���ܸ� ������.
    {
        throw std::out_of_range("minFeatureLevel to high");
    }

    // #. ����̽� ����
    // 1. ������� ��ǻ�Ϳ��� ��� �׷���ī�� ����
    // 2. ���� ������ ���� ����͸� ����
    // 3. ����̽� ������ ����
    //      => ������ ��� : ����
    //      => ����� ��� : WARP
    // 4. ���������� ����̽� ������ ���� ���� ������
    ComPtr<IDXGIAdapter1> adapter;  // ������� ���� ���� �׷��� ī��
    GetHardwareAdapter(adapter.GetAddressOf());

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

// #3. ����̽� ����
    HRESULT hr = E_FAIL;
    if (adapter)
    {
        hr = D3D11CreateDevice(
            adapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            creationFlags,
            s_featureLevels,
            featLevelCount,
            D3D11_SDK_VERSION,
            device.GetAddressOf(),
            &m_d3dFeatureLevel,
            context.GetAddressOf()
        );
    }
#if defined(NDEBUG) // ���� ȯ���� ����ȭ : assert ����� �����ش�. ����� ��忡�� ���ܰ� �߻��� ��� assert ���â�� ǥ���ϰ� ���α׷��� ��������� NDEBUG ��ũ�ΰ� ���� ������ ���ܴ� �߻������� ���α׷��� ��������� �ʴ´�.
    else
    {
        throw std::exception("No Direct3D hardware device found");
    }
#else
// #4. �׷��� ī�尡 ���ٸ� WARP�� ����Ͽ� ����̽� ����
    if (FAILED(hr)) // �׷��� ī�带 ����� ������ ���� ���� ��� ���Ӱ� �׷��� ī�带 �����.
    {
        hr = D3D11CreateDevice(
            nullptr,    // �׷��� ī�带 ���� ����
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            creationFlags,
            s_featureLevels,
            featLevelCount,
            D3D11_SDK_VERSION,
            device.GetAddressOf(),
            &m_d3dFeatureLevel,
            context.GetAddressOf()
        );

        if (SUCCEEDED(hr))
        {
            OutputDebugStringA("Direct3D Adapter - WARP\n");
        }
    }
#endif
    ThrowIfFailed(hr);

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;   // ����� �������� ����ϰ� ������ �������̽����� �������� Ȯ��
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);   // �ɰ��� ��Ȳ���� break�� �ɾ� ��
#endif // _DEBUG
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter); // ����� ȯ���� ����
        }
    }
#endif // ! NDEBUG
    ThrowIfFailed(device.As(&m_d3dDevice)); // �پ��� ������ ����̽��� �������� üũ
    ThrowIfFailed(context.As(&m_d3dContext));
    ThrowIfFailed(context.As(&m_d3dAnnotation));

}
void DeviceResources::CreateWindowSizeDependentResources()  // ������ ũ�⿡ ���� �ٽ� �����ؾ� �ϴ� ����̽� ���ҽ����� �и�
{
    if (!m_window)  // ������ �ڵ��� ��� �ִ� ��� �Լ��� �������� Ȯ���ϰ� �������̸� throw
    {
        throw std::exception("Call SetWindow with a valid Win32 window handle");
    }

    // ����̽� ���ҽ��� ����� �����ϱ� ���� ������ ����̽� ���ҽ���(����Ÿ��, ���� ���ٽ� ���� �� ��)�� ����
    ID3D11RenderTargetView* nullView[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullView), nullView, nullptr);
    m_d3dRenderTargetView.Reset();
    m_d3dDepthStencilView.Reset();
    m_depthStencil.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = std::max<UINT>(
        static_cast<UINT>(m_outputSize.right - m_outputSize.left),
        1u  // 1���� �۾������� ����, 1 ������ ���� ������ ���� ���� ���� Ű����
        );
    const UINT backBufferHeight = std::max<UINT>(
        static_cast<UINT>(m_outputSize.bottom - m_outputSize.top),
        1u
        );
// ������ �÷��״� ��Ʈ ���� ����, ���� ��Ʈ �÷��׸� OR ������ �Ͽ� 0b0111�� ��, m_options & 0b0111���� 0�� �ƴ� ��츦 �����ϸ� m_options���� �״�� ��� ��� ���� ��
//      => 3���� �ɼ� �� �� ���� ���� �ִٸ� NoSRGB()�� ���� ������ UNORM ���·� �����ϰ�, �ϳ��� �����Ǿ� ���� ������ ������ �״�� ����
    const DXGI_FORMAT backBufferFormat =
        (m_options & (c_FlipPresent | c_AllowTearing | c_EnableHDR)) ?
        NoSRGB(m_backBufferFormat) : m_backBufferFormat;

    if (m_swapChain)    // ����ü���� �����ϴ��� Ȯ���ϰ� ó���̸� ����, �ƴϸ� ������ ����
    {
        HRESULT hr = m_swapChain->ResizeBuffers(
            m_backBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
        );  //  => �׾ �÷��� Ȯ��

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ?
                    m_d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif // _DEBUG
            HandleDeviceLost();

            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }
    else
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect =
            (m_options & (c_FlipPresent | c_AllowTearing | c_EnableHDR)) ?
            DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags =
            (m_options & c_AllowTearing) ?
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr, // �⺻ ����� ����
            m_swapChain.ReleaseAndGetAddressOf()
        )); // ����ڰ� ��� ����͸� ��� �� �� GPU�� �� ���� ����Ϳ� ���� Buffer�� ���� �׸��� �׸���.
        //      => �̷� ���Ͽ� ������ ���ϰ� �߻��� �� �ִµ�, ����ڰ� GPU�� ����� ���� ���߾� ���� �����Ѵٸ� ������ ���ϰ� �߻����� �ʴ´�.

        ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(
            m_window, DXGI_MWA_NO_ALT_ENTER
        )); // ��üȭ�� ����Ű ALT + Enter( ���� ��� )�� ���´�.

        UpdateColorSpace();

        ThrowIfFailed(
            m_swapChain->GetBuffer(
                0, IID_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf())
            )
        );

        CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(
            D3D11_RTV_DIMENSION_TEXTURE2D, m_backBufferFormat
        );

        ThrowIfFailed(
            m_d3dDevice->CreateRenderTargetView
            (
                m_renderTarget.Get(),
                &renderTargetViewDesc,
                m_d3dRenderTargetView.ReleaseAndGetAddressOf()
            )
        );

        if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
        {
            CD3D11_TEXTURE2D_DESC depthStencilDesc(
                m_depthBufferFormat,
                backBufferWidth,
                backBufferHeight,
                1,
                1,
                D3D11_BIND_DEPTH_STENCIL
            );
            ThrowIfFailed(m_d3dDevice->CreateTexture2D(
                &depthStencilDesc,
                nullptr,
                m_depthStencil.ReleaseAndGetAddressOf()
            ));
        }

        m_screenViewport = CD3D11_VIEWPORT(
            0.0f,
            0.0f,
            static_cast<float>(backBufferWidth),
            static_cast<float>(backBufferHeight)
        );
    }
}
void DeviceResources::SetWindow(HWND window, int width, int height) noexcept
{
    m_window = window;

    m_outputSize.left = m_outputSize.top = 0;
    m_outputSize.right = width;
    m_outputSize.bottom = height;   // �����찡 �����Ǿ��� �� ȣ��Ǿ� ������ �ڵ�� ��� ����� ����
}
bool DeviceResources::WindowSizeChanged(int width, int height)
{
    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = width;
    newRc.bottom = height;

    if (newRc == m_outputSize)  // ���� ���� ��� ������ ���� ũ���� ���
    {
        UpdateColorSpace(); // HDR ������ ����Ϳ��� HDR ���� ����ͷ� �Ű� ���� ���� �����Ƿ� ��������� ����

        return false;
    }

    m_outputSize = newRc;   // ũ�Ⱑ ����Ǿ��� ���
    CreateWindowSizeDependentResources();
    return true;
}
void DeviceResources::HandleDeviceLost()
{
// #. ����̽��� �ջ�Ǿ��� ���
//  1. m_deviceNotify->OnDeviceLost() ����
//  2. ����̽� ���� ���ҽ� ����
//      => ���� ���ٽ� �� & ����
//      => ����Ÿ�� �� & ����
//      => ����ü��
//      => ����̽� ���ؽ�Ʈ
//      => ����̽�
//      => DXGI ���丮
//  3. ����̽� ���� ���ҽ� ����
//  4. ������ ������ ���� ���ҽ� ����
//  5. m_deviceNotify->OnDeviceRestored() ����
    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceLost();
    }

    m_d3dDepthStencilView.Reset();
    m_d3dRenderTargetView.Reset();
    m_renderTarget.Reset();
    m_depthStencil.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dAnnotation.Reset();

#ifdef _DEBUG
    {
        ComPtr<ID3D11Debug> d3dDebug;
        if (SUCCEEDED(m_d3dDevice.As(&d3dDebug)))
        {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
        }   // �ջ�� ����̽��� �ٽ� �����ϱ� ���� ��������� ����̽� �۵��ð��� ���� ������ ���� ������ ��, ���� ����
    }
#endif

    m_d3dDevice.Reset();
    m_dxgiFactory.Reset();

    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeivecRestored();
    }
}

void DX::DeviceResources::Present() // ȭ�鿡 ǥ��
{
    HRESULT hr = E_FAIL;
    if (m_options & c_AllowTearing)
    {
        hr = m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    else
    {
        hr = m_swapChain->Present(1, 0);
    }

    m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

    if (m_d3dDepthStencilView)
    {
        m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());
    }

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
            static_cast<unsigned int>(
                (hr==DXGI_ERROR_DEVICE_REMOVED) ?
                m_d3dDevice->GetDeviceRemovedReason() : hr)
        );
        OutputDebugStringA(buff);   // ����̽��� �ջ�Ǹ� ����� ��忡���� ���â�� �޽����� ���
#endif
        HandleDeviceLost(); // ����̽� �ջ� ����
    }
    else
    {
        ThrowIfFailed(hr);

        if (!m_dxgiFactory->IsCurrent())    // Cache : ���� ���� �����͸� �̸� �÷����� �װ����� �ε��� ������ ����
        {   // ���̷�ƮX�� Cache �� ����
            CreateFactory();
        }
    }
}

void DeviceResources::CreateFactory()
{
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603) // ����� ����̰� �ü���� Windows 8.1 �̻��̸�
    bool debugDXGI = false;
    {
        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;

        // ����׿� �������̽��� ������ ��� CreateDXGIFactory2() ���� -> ����� �ߴ��� ���� -> ����� �޽��� ���͸�
        if (SUCCEEDED(DXGIGetDebugInterface1(
            0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())
        )))
        {
            debugDXGI = true;

            ThrowIfFailed(CreateDXGIFactory2(
                DXGI_CREATE_FACTORY_DEBUG,
                IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())
            ));

            dxgiInfoQueue->SetBreakOnSeverity(
                DXGI_DEBUG_ALL,
                DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR,
                true
            );
            dxgiInfoQueue->SetBreakOnSeverity(
                DXGI_DEBUG_ALL,
                DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION,
                true
            );

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] = { 80 }; // SwapChin - GetContainingQutput() : ���� �����찡 ���ԵǾ� �ִ� �����츦 ã�ƿ��� �Լ�
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
    }

    if (!debugDXGI)
#endif
        // ������ ����̰ų� �ü���� Windows 8.1 �̸��̸� CreateDXGIFactory1() ����
        ThrowIfFailed(CreateDXGIFactory1(
            IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())
        ));
}
void DeviceResources::GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
{
}
void DeviceResources::UpdateColorSpace()
{
}
