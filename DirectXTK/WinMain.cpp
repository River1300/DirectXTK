#include "pch.h"

// #. 링커 파일
//		=> 지금까지는 헤더파일에 #pragma 키워드를 사용해 라이브러리 파일을 링크했다.
//	1. 프로젝트명 -> 속성
//	2. 모든 구성 + 모든 플랫폼
//	3. 구성 속성 -> 링커 -> 입력 항목
//	4. 추가 종속성 -> 편집
//	5. 추가 종속성 .lib를 추가

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	DX::StepTimer timer;

	return 0;
}
