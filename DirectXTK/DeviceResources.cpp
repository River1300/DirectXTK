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

// #1. 디버그 체크
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

// #2. 옵션에 따라 기능 지원 여부를 체크
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

// #3. 디바이스 생성
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
// #4. 그래픽 카드가 없다면 WARP을 사용하여 디바이스 생성
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
void DeviceResources::CreateWindowSizeDependentResources()  // 윈도우 크기에 따라 다시 생성해야 하는 디바이스 리소스들을 분리
{
    if (!m_window)  // 윈도우 핸들을 담고 있는 멤버 함수가 정상인지 확인하고 비정상이면 throw
    {
        throw std::exception("Call SetWindow with a valid Win32 window handle");
    }

    // 디바이스 리소스의 사이즈를 변경하기 위해 기존의 디바이스 리소스들(렌더타겟, 깊이 스텐실 버퍼 및 뷰)을 리셋
    ID3D11RenderTargetView* nullView[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullView), nullView, nullptr);
    m_d3dRenderTargetView.Reset();
    m_d3dDepthStencilView.Reset();
    m_depthStencil.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = std::max<UINT>(
        static_cast<UINT>(m_outputSize.right - m_outputSize.left),
        1u  // 1보다 작아질리가 없다, 1 이하의 값이 들어오는 것을 맊기 위한 키워드
        );
    const UINT backBufferHeight = std::max<UINT>(
        static_cast<UINT>(m_outputSize.bottom - m_outputSize.top),
        1u
        );
// 각각의 플래그는 비트 별로 구분, 오든 비트 플래그를 OR 연산을 하여 0b0111이 됨, m_options & 0b0111에서 0이 아닌 경우를 제외하면 m_options값이 그대로 계산 결과 값이 됨
//      => 3가지 옵션 중 한 개라도 켜져 있다면 NoSRGB()를 통해 포멧을 UNORM 형태로 변경하고, 하나도 설정되어 있지 않으면 포멧을 그대로 유지
    const DXGI_FORMAT backBufferFormat =
        (m_options & (c_FlipPresent | c_AllowTearing | c_EnableHDR)) ?
        NoSRGB(m_backBufferFormat) : m_backBufferFormat;

    if (m_swapChain)    // 스왑체인이 존재하는지 확인하고 처음이면 생성, 아니면 사이즈 변경
    {
        HRESULT hr = m_swapChain->ResizeBuffers(
            m_backBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
        );  //  => 테어링 플래그 확인

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
            nullptr, // 기본 모니터 제공
            m_swapChain.ReleaseAndGetAddressOf()
        )); // 사용자가 듀얼 모니터를 사용 할 때 GPU는 두 개의 모니터에 각각 Buffer를 통해 그림을 그린다.
        //      => 이로 인하여 성능의 저하가 발생할 수 있는데, 사용자가 GPU를 모니터 수에 맞추어 각각 적용한다면 성능의 저하가 발생하지 않는다.

        ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(
            m_window, DXGI_MWA_NO_ALT_ENTER
        )); // 전체화면 단축키 ALT + Enter( 독점 모드 )를 막는다.

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
    m_outputSize.bottom = height;   // 윈도우가 생성되었을 때 호출되어 윈도우 핸들과 출력 사이즈를 갱신
}
bool DeviceResources::WindowSizeChanged(int width, int height)
{
    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = width;
    newRc.bottom = height;

    if (newRc == m_outputSize)  // 만약 기존 출력 영역과 같은 크기일 경우
    {
        UpdateColorSpace(); // HDR 미지원 모니터에서 HDR 지원 모니터로 옮겨 졌을 수도 있으므로 색상공간을 갱신

        return false;
    }

    m_outputSize = newRc;   // 크기가 변경되었을 경우
    CreateWindowSizeDependentResources();
    return true;
}
void DeviceResources::HandleDeviceLost()
{
// #. 디바이스가 손상되었을 경우
//  1. m_deviceNotify->OnDeviceLost() 전달
//  2. 디바이스 종속 리소스 리셋
//      => 깊이 스텐실 뷰 & 버퍼
//      => 렌더타겟 뷰 & 버퍼
//      => 스왑체인
//      => 디바이스 컨텍스트
//      => 디바이스
//      => DXGI 팩토리
//  3. 디바이스 종속 리소스 생성
//  4. 윈도우 사이즈 종속 리소스 생성
//  5. m_deviceNotify->OnDeviceRestored() 전달
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
        }   // 손상된 디바이스를 다시 생성하기 전에 현재까지의 디바이스 작동시간에 대한 정보를 먼저 보고한 후, 새로 생성
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

void DX::DeviceResources::Present() // 화면에 표시
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
        OutputDebugStringA(buff);   // 디바이스가 손상되면 디버그 모드에서만 출력창에 메시지를 출력
#endif
        HandleDeviceLost(); // 디바이스 손상에 대응
    }
    else
    {
        ThrowIfFailed(hr);

        if (!m_dxgiFactory->IsCurrent())    // Cache : 자주 쓰는 데이터를 미리 올려놓고 그곳에서 로딩을 빠르게 진행
        {   // 다이렉트X가 Cache 를 관리
            CreateFactory();
        }
    }
}

void DeviceResources::CreateFactory()
{
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603) // 디버그 모드이고 운영체제가 Windows 8.1 이상이면
    bool debugDXGI = false;
    {
        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;

        // 디버그용 인터페이스가 존재할 경우 CreateDXGIFactory2() 생성 -> 디버그 중단점 지정 -> 디버그 메시지 필터링
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

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] = { 80 }; // SwapChin - GetContainingQutput() : 현재 윈도우가 포함되어 있는 윈도우를 찾아오는 함수
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
    }

    if (!debugDXGI)
#endif
        // 릴리즈 모드이거나 운영체제가 Windows 8.1 미만이면 CreateDXGIFactory1() 생성
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
