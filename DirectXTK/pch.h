#pragma once

// #. DirectXToolKit
//		=> DirectXTK는 어렵고 복잡한 DirectX 기능들을 함수로 묶어 주는 헬퍼 라이브러리 이다. 참고로 과거 버전에는 D3DX 라는
//		=> 라이브러리가 존재 했으며 이런 버전은 더 이상 지원되지 않는다.
// 설치 : 프로젝트 -> NuGet 패키지 관리 -> 찾아보기 -> DirectXTK -> DirectXTK_Desctop_win 10 설치

// #. DirectXTK 프레임워크 살펴보기
//	미리 컴파일된 헤더( Pre - Compiled Header )
//		=> 미리 컴파일된 헤더라는 개념으로 컴파일 속도를 향상 시키는 기법 중 하나
//		=> #include, inline 등의 기능은 컴파일 때 복사/붙여넣기와 비슷한 작업들이 매법 발생하게 된다.
//		=> 미리 컴파일된 헤더는 이런 헤더들을 한 곳에 모아 한 번 컴파일 해 놓고 다음 번에는 컴파일 된 내용을 직접 사용하는 것
//		=> 이런 식으로 컴파일 속도를 향상 시키는 것, 주로 시스템 헤더( iostream, vector 등 표준 시스템 포함 파일 ) 및
//		=> 자주 사용되지만 자주 변경되지 않는 프로젝트 포함 파일( Framework 처럼 한 번 만들고 나면 자주 수정되지 않는 헤더 ) 들을
//		=> 모아 놓으면 컴파일 속도를 단축할 수 있다.

// 기본적으로 Header는 컴파일이 않되기 때문에 cpp 파일을 동일한 이름으로 만든다.

// #. Header for standard system include files.
#include <winsdkver.h>	// 1. Windows Software Development Kit Version의 약자로 윈도우 개발 킷의 버전을 관리하는 헤더 파일
#define _WIN32_WINNT 0x0A00	// 2. 앱이 실행될 때 어떤 버전의 윈도우에서 실행되는지를 지정할 수 있다. Window 10 = 0x0A00
#include <sdkddkver.h>	// 3. Software Development Kit Driver Development Kit Version의 약자로 앱 및 드라이버 개발 도구의 버전을 모두 관리하는 헤더

// #. Use the C++ standard templated min/max
#define NOMINMAX	// 4. Windows 헤더에도 min, max 가 존재하는데 STL에도 min, max가 있어서 이름 충돌이 발생
//		=> 따라서 이 매크로를 선언하면 Windows에서 정의한 min/max가 선언해제된다.
//		=> 그래야 STL( C++ )에 있는 min/max를 사용할 수 있다.

// #. DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP	// 5. DirectX를 사용할 것이기 때문에 GDI에서 제공하는 기본 기능을 다이어트 시킨다.

// #. Include <mcx.h> if you need this
#define NOMCX		// 6. Model Configuration Extensions의 약자로 과거 랜카드 이전 세대인 모뎀에 대한 지원을 다이어트 시킨다.

// #. Include <winsvc.h> if you need this
#define NOSERVICE	// 7. 윈도우 앱에서의 서비스(윈도우 창이 없이 보이지 않는 기본 작업들) 프로그램도 다이어트
//		=> 보안 프로그램, 인증 프로그램 등등

// #. WinHelp is deprecated
#define NOHELP	// 8. F1키( 윈도우 기본 도움말 )도 다이어트

#define WIN32_LEAN_AND_MEAN	// 9. 불필요한 Cryptography, DDE, RPC, Shell, Windows Soket 등을 추가로 다이어트 하여 게임 만드는데 집중할 수 있다.
//		=> 여기까지 불필요한 헤더를 제거해 컴파일 속도를 빠르게 개선

#include <Windows.h>

#include <wrl/client.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>
#include <DirectXColors.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>

#include <stdio.h>

#ifdef _DEBUG	// #ifdef 만약 DEBUG로 정의 되어 있다면 아래 항목을 실행하고 RELEASE 모드로 정의되면 실행하지 않는다.
#include <dxgidebug.h>
#endif

// #. 경고 메시지에 대한 예외 처리를 스택에 Push, Pop 한다. 경고 처리는 필요할ㄹ 때 제거하고 해제할 수 있어야 하는데, 이를 스택 형태로 구현
#pragma warning(push)
#pragma warning(disable:26812)	// enum 대신에 enum class를 사용하라는 경고를 제거
// 경고 수준 조절 : 솔루션 탐색기 -> 프로젝트 명 -> 속성 -> C/C++ -> 일반 -> 경고 수준 || 고급 -> 특정 경고 사용 안 함

// #. DirectXTK
#include "Audio.h"
#include "CommonStates.h"
#include "WICTextureLoader.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PrimitiveBatch.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"

#pragma warning(pop)

// DX Sample
#include "StepTimer.h"

// #. 예외 처리 클래스
namespace DX
{
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) noexcept : result(hr) {}

		const char* what() const override
		{
			static char s_str[128] = {};
			sprintf_s(
				s_str,
				"Failure with HRESULT of %08X",
				static_cast<unsigned int>(result)
			);
			return s_str;
		}

	private:
		HRESULT result;
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}
}

// #. 미리 컴파일된 헤더 적용하기
//		1. 솔루션 탐색기 -> 프로젝트명 -> 속성
//		2. 좌측 상단 -> 구성 -> 모든 구성
//		3. 플렛폼 -> 모든 플렛폼
//		4. C/C++ -> 미리 컴파일된 헤더 -> 사용
//		5. 미리 컴파일된 헤더 파일 -> 헤더 파일 명 입력
//		6. 헤더 파일( pch.h )과 연결된 파일( pch.cpp )의 속성 -> C/C++ -> 미리 컴파일된 헤더 -> 만들기
//	=> 해당 프로젝트의 모든 .cpp 파일에는 헤더 파일( pch.h )이 포함되어야 한다.

//	=> 툴바에서 디버그용 및 배포용 빌드를 선택할 수 있으며, 32비트(x86) 및 64비트(x64)용 빌드를 선택할 수 있다.
// #. Debug : 개발용, 링크할 때 추가 정보를 포함시켜 디버깅이 가능해 진다. 당연히 실행파일의 크기도 커진다.
// #. Release : 배포용, 추가 정보는 모두 제거하여, 최적화를 수행한다.
// #. x64 : 64비트 운영체제에서 실행되는 64비트 앱을 만든다.
// #. x86 : 32비트 운영체제에서 실행되는 32비트 앱을 만든다. 32비트 앱이 64비트 운영체제에서 실행될 때는 WOW64라는 에뮬레이터를 통해 실행된다. 비슷한 성능을 보여주긴 하지만, 부하가 발생할 수 있다.
//		=> 64비트 운영체제는 32비트 운영체제 보다 더 많은 리소스를 사용할 수 있다.
// #. 32비트 윈도우 : 앱이 사용가능한 메모리[ 최대 3GByte ], 한 번에 4바이트씩 연산
// #. 64비트 윈도우 : 앱이 사용가능한 메모리[ 최대 2TByte ], 한 번에 8바이트씩 연산
