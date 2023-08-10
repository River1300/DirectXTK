#pragma once

// #. DirectXToolKit
//		=> DirectXTK�� ��ư� ������ DirectX ��ɵ��� �Լ��� ���� �ִ� ���� ���̺귯�� �̴�. ����� ���� �������� D3DX ���
//		=> ���̺귯���� ���� ������ �̷� ������ �� �̻� �������� �ʴ´�.
// ��ġ : ������Ʈ -> NuGet ��Ű�� ���� -> ã�ƺ��� -> DirectXTK -> DirectXTK_Desctop_win 10 ��ġ

// #. DirectXTK �����ӿ�ũ ���캸��
//	�̸� �����ϵ� ���( Pre - Compiled Header )
//		=> �̸� �����ϵ� ������ �������� ������ �ӵ��� ��� ��Ű�� ��� �� �ϳ�
//		=> #include, inline ���� ����� ������ �� ����/�ٿ��ֱ�� ����� �۾����� �Ź� �߻��ϰ� �ȴ�.
//		=> �̸� �����ϵ� ����� �̷� ������� �� ���� ��� �� �� ������ �� ���� ���� ������ ������ �� ������ ���� ����ϴ� ��
//		=> �̷� ������ ������ �ӵ��� ��� ��Ű�� ��, �ַ� �ý��� ���( iostream, vector �� ǥ�� �ý��� ���� ���� ) ��
//		=> ���� �������� ���� ������� �ʴ� ������Ʈ ���� ����( Framework ó�� �� �� ����� ���� ���� �������� �ʴ� ��� ) ����
//		=> ��� ������ ������ �ӵ��� ������ �� �ִ�.

// �⺻������ Header�� �������� �ʵǱ� ������ cpp ������ ������ �̸����� �����.

// #. Header for standard system include files.
#include <winsdkver.h>	// 1. Windows Software Development Kit Version�� ���ڷ� ������ ���� Ŷ�� ������ �����ϴ� ��� ����
#define _WIN32_WINNT 0x0A00	// 2. ���� ����� �� � ������ �����쿡�� ����Ǵ����� ������ �� �ִ�. Window 10 = 0x0A00
#include <sdkddkver.h>	// 3. Software Development Kit Driver Development Kit Version�� ���ڷ� �� �� ����̹� ���� ������ ������ ��� �����ϴ� ���

// #. Use the C++ standard templated min/max
#define NOMINMAX	// 4. Windows ������� min, max �� �����ϴµ� STL���� min, max�� �־ �̸� �浹�� �߻�
//		=> ���� �� ��ũ�θ� �����ϸ� Windows���� ������ min/max�� ���������ȴ�.
//		=> �׷��� STL( C++ )�� �ִ� min/max�� ����� �� �ִ�.

// #. DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP	// 5. DirectX�� ����� ���̱� ������ GDI���� �����ϴ� �⺻ ����� ���̾�Ʈ ��Ų��.

// #. Include <mcx.h> if you need this
#define NOMCX		// 6. Model Configuration Extensions�� ���ڷ� ���� ��ī�� ���� ������ �𵩿� ���� ������ ���̾�Ʈ ��Ų��.

// #. Include <winsvc.h> if you need this
#define NOSERVICE	// 7. ������ �ۿ����� ����(������ â�� ���� ������ �ʴ� �⺻ �۾���) ���α׷��� ���̾�Ʈ
//		=> ���� ���α׷�, ���� ���α׷� ���

// #. WinHelp is deprecated
#define NOHELP	// 8. F1Ű( ������ �⺻ ���� )�� ���̾�Ʈ

#define WIN32_LEAN_AND_MEAN	// 9. ���ʿ��� Cryptography, DDE, RPC, Shell, Windows Soket ���� �߰��� ���̾�Ʈ �Ͽ� ���� ����µ� ������ �� �ִ�.
//		=> ������� ���ʿ��� ����� ������ ������ �ӵ��� ������ ����

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

#ifdef _DEBUG	// #ifdef ���� DEBUG�� ���� �Ǿ� �ִٸ� �Ʒ� �׸��� �����ϰ� RELEASE ���� ���ǵǸ� �������� �ʴ´�.
#include <dxgidebug.h>
#endif

// #. ��� �޽����� ���� ���� ó���� ���ÿ� Push, Pop �Ѵ�. ��� ó���� �ʿ��Ҥ� �� �����ϰ� ������ �� �־�� �ϴµ�, �̸� ���� ���·� ����
#pragma warning(push)
#pragma warning(disable:26812)	// enum ��ſ� enum class�� ����϶�� ��� ����
// ��� ���� ���� : �ַ�� Ž���� -> ������Ʈ �� -> �Ӽ� -> C/C++ -> �Ϲ� -> ��� ���� || ��� -> Ư�� ��� ��� �� ��

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

// #. ���� ó�� Ŭ����
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

// #. �̸� �����ϵ� ��� �����ϱ�
//		1. �ַ�� Ž���� -> ������Ʈ�� -> �Ӽ�
//		2. ���� ��� -> ���� -> ��� ����
//		3. �÷��� -> ��� �÷���
//		4. C/C++ -> �̸� �����ϵ� ��� -> ���
//		5. �̸� �����ϵ� ��� ���� -> ��� ���� �� �Է�
//		6. ��� ����( pch.h )�� ����� ����( pch.cpp )�� �Ӽ� -> C/C++ -> �̸� �����ϵ� ��� -> �����
//	=> �ش� ������Ʈ�� ��� .cpp ���Ͽ��� ��� ����( pch.h )�� ���ԵǾ�� �Ѵ�.

//	=> ���ٿ��� ����׿� �� ������ ���带 ������ �� ������, 32��Ʈ(x86) �� 64��Ʈ(x64)�� ���带 ������ �� �ִ�.
// #. Debug : ���߿�, ��ũ�� �� �߰� ������ ���Խ��� ������� ������ ����. �翬�� ���������� ũ�⵵ Ŀ����.
// #. Release : ������, �߰� ������ ��� �����Ͽ�, ����ȭ�� �����Ѵ�.
// #. x64 : 64��Ʈ �ü������ ����Ǵ� 64��Ʈ ���� �����.
// #. x86 : 32��Ʈ �ü������ ����Ǵ� 32��Ʈ ���� �����. 32��Ʈ ���� 64��Ʈ �ü������ ����� ���� WOW64��� ���ķ����͸� ���� ����ȴ�. ����� ������ �����ֱ� ������, ���ϰ� �߻��� �� �ִ�.
//		=> 64��Ʈ �ü���� 32��Ʈ �ü�� ���� �� ���� ���ҽ��� ����� �� �ִ�.
// #. 32��Ʈ ������ : ���� ��밡���� �޸�[ �ִ� 3GByte ], �� ���� 4����Ʈ�� ����
// #. 64��Ʈ ������ : ���� ��밡���� �޸�[ �ִ� 2TByte ], �� ���� 8����Ʈ�� ����
