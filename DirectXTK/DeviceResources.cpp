#include "pch.h"	// 프리 컴파일 헤더는 세팅을 하면 .cpp를 만들 때 자동으로 추가된다.
#include "DeviceResources.h"

using namespace DirectX;
using namespace DX;
using Microsoft::WRL::ComPtr;

// #. clang( 클랭 ): C, C++, Object-C 언어를 위한 컴파일러
//      => 컴파일러 프론트엔드 : 백엔드로 LLVM이란 컴파일러를 사용한다.
//      => 오픈소스인 GNU Compiler를 대체하기 위해 등장했으며 유닉스 계열에서 주로 사용하는 컴파일러
//      => __clang__이 선언되어 있다면 ifedf ~ endif 안에 있는 기능을 실행하고 아니면 무시
#ifdef __clang__
#pragma clang diagnostic ignored "_Wcovered-switch-detaulf" // 윈도우 운영체제가 아닌 다른 운영체제에서 사용할 수 있는 C, C++...
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace  // 이름 없는 네임스페이스 : 이 공간에서만 사용할 수 있는 변수
{
#if defined(_DEBUG) // Debug 모드로 빌드할 때
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
        return SUCCEEDED(hr);   // 인라인 함수로 디버그모드로 사용 가능한 DirectX 디바이스가 존재하는지 체크
    }
#endif

    // 이름대로 DXGI_FORMAT을 SRGB모드가 아닌 포멧으로 변경하는 함수
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

void DeviceResources::CreateDeviceResources()   // 사용자 컴퓨터 환경을 체크하여 최적의 게임 환경을 만들어 준다.
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;  // 사용자의 그래픽카드가 BGRA 포멧 형식이 지원되는지 확인

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
        BOOL allowTearing = FALSE;  // 테어링 지원 여부

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = m_dxgiFactory.As(&factory5);   // 인터페이스가 Factory5를 지원하는지 확인
        //  => 하드웨어 테어링 허용 옵션은 dxgi1_5.h에서 지원되며 IDXGFactory5 인터페이스에서 체크가 가능하다.
        //      => 현재 dxgi 이너페이스가 IDXGIFactory5에 다형성을 가리고 있으면 ( as - a ) hr에 S_OK가 반환되고
        //          => Factory5에 m_dxgiFactory를 변환하여 대입해 준다.

        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing,
                sizeof(allowTearing)
            );
        }
// 테어링은 그래픽 손상처럼 보이는 좋지 않은 것이지만, 최근 가변 주파수 모니터의 등장으로 개념이 바뀌었다. 기존에는 V-Sync( 수직 동기화 )
// 로 그래픽카드가 모니터를 기다리는 방식이었지만 고사양 모니터들은 오히려 그래픽카드를 기다리는 방식을 사용한다. 따라서 그래픽 카드는 최대한
// 빨리 업데이트를 해줘야 하는데 이를 위해서는 Tearing 허용 즉 수직 동기화를 꺼줘야 한다. 물론 이렇게 동작하는 만큼 모니터는
// 최대한 빠른 주파수를 가지는 것이 좋다. ( 요즘 75Hz 이상의 모니터들이 많은데 이런 모니터를 지원하려면 이 기능을 체크해 줘야 한다. )

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

    if (m_options & c_EnableHDR)    // HDR 지원 확인, 디바이스가 아닌 모니터를 체크하는 것으로 단순히 DXGI 버전만 확인
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

    if (m_options & c_FlipPresent)  // 플립 프레젠테이션 효과 지원 확인
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

    // 지원되는 DirectX 버전을 확인
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
    }   // 넘겨 받은 DX 버전들 중 옵션으로 지정한 Level보다 큰 버전들의 갯수만 구한다.
    //      => 배열은 큰 버전 부터 내림차순으로 정렬되어 있으므로 처음 Level보다 작은 값을 만나면 반복문을 종료한다.

    if (!featLevelCount)    // 최소 요구사항이 너무 높으면 예외를 던진다.
    {
        throw std::out_of_range("minFeatureLevel to high");
    }

    // #. 디바이스 생성
    // 1. 사용자의 컴퓨터에서 모든 그래픽카드 나열
    // 2. 가장 성능이 좋은 어댑터를 선택
    // 3. 디바이스 생성에 실패
    //      => 릴리즈 모드 : 종료
    //      => 디버그 모드 : WARP
    // 4. 최종적으로 디바이스 생성에 실패 예외 던지기
    ComPtr<IDXGIAdapter1> adapter;  // 사용자의 가장 좋은 그래픽 카드
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
#if defined(NDEBUG) // 빌드 환경의 세분화 : assert 기능을 막아준다. 디버그 모드에서 예외가 발생할 경우 assert 경고창을 표시하고 프로그램이 종료되지만 NDEBUG 매크로가 켜져 있으면 예외는 발생하지만 프로그램이 종료되지는 않는다.
    else
    {
        throw std::exception("No Direct3D hardware device found");
    }
#else
    if (FAILED(hr)) // 그래픽 카드를 만드는 과정이 싪패 했을 경우 새롭게 그래픽 카드를 만든다.
    {
        hr = D3D11CreateDevice(
            nullptr,    // 그래픽 카드를 받지 않음
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
    ComPtr<ID3D11Debug> d3dDebug;   // 디버그 정보들을 출력하고 생성된 인터페이스들이 정상인지 확인
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);   // 심각한 상황에서 break를 걸어 줌
#endif // _DEBUG
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter); // 디버그 환경을 세팅
        }
    }
#endif // ! NDEBUG
    ThrowIfFailed(device.As(&m_d3dDevice)); // 다양한 버전의 디바이스의 다형성을 체크
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
