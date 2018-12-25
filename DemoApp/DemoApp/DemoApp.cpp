// DemoApp.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DemoApp.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			DemoApp app;
			if (SUCCEEDED(app.Initialize()))
			{
				app.RunMessageLoop();
			}
		}

		CoUninitialize();
	}

	return 0;
}

DemoApp::DemoApp()
	: m_hWnd(nullptr)
	, m_pDirect2dFactory(nullptr)
	, m_pRenderTarget(nullptr)
	, m_pLightSlateGrayBrush(nullptr)
	, m_pCornflowerBlueBrush(nullptr)
{

}

DemoApp::~DemoApp()
{
	SafeRelease(&m_pDirect2dFactory);
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pLightSlateGrayBrush);
	SafeRelease(&m_pCornflowerBlueBrush);
}

HRESULT DemoApp::Initialize()
{
	HRESULT hr;

	hr = CreateDeviceIndependentResources();

	if (SUCCEEDED(hr))
	{
		WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = DemoApp::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
		wcex.lpszClassName = L"D3DDemoApp";

		RegisterClassEx(&wcex);

		FLOAT dpiX, dpiY;

		m_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);

		m_hWnd = CreateWindow(L"D3DDemoApp", L"Direct2D Demo App", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, static_cast<UINT>(ceil(640.f * dpiX / 96.0f)),
			static_cast<UINT>(ceil(480.0f * dpiY) / 96.0f), NULL, NULL, HINST_THISCOMPONENT, this);
	
		hr = m_hWnd ? S_OK : S_FALSE;

		if (SUCCEEDED(hr))
		{
			ShowWindow(m_hWnd, SW_SHOWNORMAL);
			UpdateWindow(m_hWnd);
		}
	}

	return hr;
}

void DemoApp::RunMessageLoop()
{
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{

	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
	return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
	HRESULT hr = S_OK;
	if (!m_pRenderTarget)
	{
		RECT rc;
		GetClientRect(m_hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		hr = m_pDirect2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hWnd, size),
			&m_pRenderTarget);

		if (SUCCEEDED(hr))
		{
			hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray),
				&m_pLightSlateGrayBrush);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
				&m_pCornflowerBlueBrush);
		}
	}

	return hr;
}

void DemoApp::DiscardDeviceResources()
{
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pLightSlateGrayBrush);
	SafeRelease(&m_pCornflowerBlueBrush);
}

HRESULT DemoApp::OnRender()
{
	HRESULT hr = S_OK;

	hr = CreateDeviceResources();
	if (SUCCEEDED(hr))
	{
		m_pRenderTarget->BeginDraw();

		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();

		int width = static_cast<int>(rtSize.width);
		int height = static_cast<int>(rtSize.height);

		for (int x = 0; x < width; x += 10)
		{
			m_pRenderTarget->DrawLine(D2D1::Point2F(x, 0.0f),
				D2D1::Point2F(x, rtSize.height), m_pLightSlateGrayBrush, 0.5f);
		}

		for (int y = 0; y < height; y += 10)
		{
			m_pRenderTarget->DrawLine(D2D1::Point2F(0, y),
				D2D1::Point2F(rtSize.width, y), m_pLightSlateGrayBrush, 0.5f);
		}

		D2D1_RECT_F rectangle1 = D2D1::RectF(rtSize.width / 2 - 50.0f,
			rtSize.height / 2 - 50.0f,
			rtSize.width / 2 + 50.0f,
			rtSize.height / 2 + 50.0f);

		D2D1_RECT_F rectangle2 = D2D1::RectF(rtSize.width / 2 - 100.0f,
			rtSize.height / 2 - 100.0f,
			rtSize.width / 2 + 100.0f,
			rtSize.height / 2 + 100.0f);

		//m_pRenderTarget->FillRectangle(&rectangle1, m_pLightSlateGrayBrush);
		m_pRenderTarget->FillRectangle(&rectangle2, m_pCornflowerBlueBrush);

		hr = m_pRenderTarget->EndDraw();
	}

	if (hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		DiscardDeviceResources();
	}

	return hr;
}

void DemoApp::OnResize(UINT width, UINT height)
{
	if (m_pRenderTarget)
	{
		m_pRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}

LRESULT CALLBACK DemoApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	if (WM_CREATE == message)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		DemoApp *app = (DemoApp*)pcs->lpCreateParams;
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, PtrToUlong(app));
		result = 1;
	}
	else
	{
		DemoApp *app = reinterpret_cast<DemoApp*>(static_cast<LONG_PTR>(
			GetWindowLongPtr(hWnd, GWLP_USERDATA)));
		bool wasHandled = false;

		if (app)
		{
			switch (message)
			{
			case WM_SIZE:
			   {
				  UINT width = LOWORD(lParam);
				  UINT height = HIWORD(lParam);
				  app->OnResize(width, height);
			   }
			   result = 0;
			   wasHandled = true;
				break;
			case WM_DISPLAYCHANGE:
			    {
					InvalidateRect(hWnd, NULL, FALSE);
			    }
				result = 0;
				wasHandled = true;
				break;
			case WM_PAINT:
				{
					app->OnRender();
					ValidateRect(hWnd, NULL);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_DESTROY:
				{
					PostQuitMessage(0);
				}
				result = 1;
				wasHandled = true;
				break;
			default:
				break;
			}
		}

		if (!wasHandled)
		{
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return result;
}
