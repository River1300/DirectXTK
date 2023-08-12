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

#if defined(_DEBUG)
    if (SdkLayerAvailable())
    {
        creationFlags != D3D11_CREATE_DEVICE_DEBUG;
    }
    else
    {
        OutputDebugStringA("WARNING: Direct3D Debug Device is not available \n");
    }
#endif

    CreateFactory();

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
void DeviceResources::CreateWindowSizeDependentResources()
{
}
void DeviceResources::SetWindow(HWND window, int width, int height) noexcept
{
}
bool DeviceResources::WindowSizeChanged(int width, int height)
{
    return false;
}
void DeviceResources::HandleDeviceLost()
{
}
void DeviceResources::CreateFactory()
{
}
void DeviceResources::GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
{
}
void DeviceResources::UpdateColorSpace()
{
}
