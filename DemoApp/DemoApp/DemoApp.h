#pragma once

#include "resource.h"

#include <Windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

template <typename Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = nullptr;
	}
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!b) {OutputDebugStringA("Assert: "#b "\n");}} while(0)
#else
#define  Assert(b)
#endif
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class DemoApp
{
public:
	DemoApp();
	~DemoApp();

	HRESULT Initialize();
	void RunMessageLoop();

private:
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();

	void DiscardDeviceResources();

	HRESULT  OnRender();

	void OnResize(UINT width, UINT height);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND m_hWnd;
	ID2D1Factory *m_pDirect2dFactory;
	ID2D1HwndRenderTarget *m_pRenderTarget;
	ID2D1SolidColorBrush *m_pLightSlateGrayBrush;
	ID2D1SolidColorBrush *m_pCornflowerBlueBrush;
};