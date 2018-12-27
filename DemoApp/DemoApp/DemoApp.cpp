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
	, m_pDirectWFactory(nullptr)
	, m_pWICImagingFactory(nullptr)
	, m_pWriteTextFormat(nullptr)
	, m_pRenderTarget(nullptr)
	, m_pLightSlateGrayBrush(nullptr)
	, m_pCornflowerBlueBrush(nullptr)
{

}

DemoApp::~DemoApp()
{
	SafeRelease(&m_pDirect2dFactory);
	SafeRelease(&m_pDirectWFactory);
	SafeRelease(&m_pWICImagingFactory);
	SafeRelease(&m_pWriteTextFormat);
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
	if (FAILED(hr))
	{
		return hr;
	}

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDirectWFactory));
	if (FAILED(hr))
		return hr;

	hr = m_pDirectWFactory->CreateTextFormat(L"Microsoft YaHei UI",
		nullptr,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL, 50.0f, L"zh-cn", &m_pWriteTextFormat);
	if (FAILED(hr))
		return hr;

	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICImagingFactory));
	if (FAILED(hr))
		return hr;

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

HRESULT DemoApp::LoadBitmapFromFile(ID2D1RenderTarget *pRenderTarget, IWICImagingFactory *pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap **ppBitmap)
{
	HRESULT hr = S_FALSE;

	IWICBitmapDecoder *pBitmapDecoder = NULL;
	IWICBitmapFrameDecode *pBitmapFrame = NULL;
	IWICFormatConverter *pConvert = NULL;
	IWICBitmapScaler *pScaler = nullptr;

	hr = pIWICFactory->CreateDecoderFromFilename(uri,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad, &pBitmapDecoder);

	if (FAILED(hr))
		goto cleanup;

	hr = pBitmapDecoder->GetFrame(0, &pBitmapFrame);
	if (FAILED(hr))
		goto cleanup;

	hr = pIWICFactory->CreateFormatConverter(&pConvert);
	if (FAILED(hr))
		goto cleanup;

	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pBitmapFrame->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr))
			goto cleanup;

		if (destinationWidth == 0)
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0)
		{
			FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
			destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
		}

		hr = pIWICFactory->CreateBitmapScaler(&pScaler);
		if (SUCCEEDED(hr))
		{
			hr = pScaler->Initialize(pBitmapFrame, destinationWidth,
				destinationHeight, WICBitmapInterpolationModeCubic);
		}

		if (SUCCEEDED(hr))
		{
			hr = pConvert->Initialize(pScaler,
				GUID_WICPixelFormat32bppPRGBA,
				WICBitmapDitherTypeNone,
				NULL,
				0.0f,
				WICBitmapPaletteTypeMedianCut);
		}

		if (SUCCEEDED(hr))
		{
			hr = pRenderTarget->CreateBitmapFromWicBitmap(pConvert,
				NULL, ppBitmap);
		}
	}

cleanup:
	SafeRelease(&pBitmapDecoder);
	SafeRelease(&pBitmapFrame);
	SafeRelease(&pConvert);
	SafeRelease(&pScaler);

	return hr;
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

		//»æÖÆ¾ØÐÎ
		m_pRenderTarget->FillRectangle(&rectangle2, m_pCornflowerBlueBrush);

		// »æÖÆÔ²½Ç¾ØÐÎ
		/*D2D1_ROUNDED_RECT rc;
		rc.rect = rectangle2;
		rc.radiusX = 30;
		rc.radiusY = 30;
		m_pRenderTarget->DrawRoundedRectangle(rc, m_pCornflowerBlueBrush);*/

		// »æÖÆÍÖÔ²
		/*D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(100.0f, 100.0f), 50, 50);
		m_pRenderTarget->FillEllipse(ellipse, m_pCornflowerBlueBrush);*/


		// »æÖÆÎÄ×Ö
		/*RECT rc;
		GetClientRect(m_hWnd, &rc);
		D2D1_RECT_F textLayoutRect = D2D1::RectF(
			static_cast<FLOAT>(rc.left),
			static_cast<FLOAT>(rc.top),
			static_cast<FLOAT>(rc.right - rc.left),
			static_cast<FLOAT>(rc.bottom - rc.top)
		);
		WCHAR tsz[100] = L"wenxinzhou";
		m_pRenderTarget->DrawTextW(tsz, 100, m_pWriteTextFormat, textLayoutRect,
			m_pCornflowerBlueBrush);*/

		// »æÖÆÍ¼Æ¬
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		D2D1_RECT_F imageLayoutRect = D2D1::RectF(
			static_cast<FLOAT>(rc.left),
			static_cast<FLOAT>(rc.top),
			static_cast<FLOAT>(rc.right - rc.left),
			static_cast<FLOAT>(rc.bottom - rc.top)
		);

		ID2D1Bitmap *pBitmap = nullptr;
		if (LoadBitmapFromFile(m_pRenderTarget, m_pWICImagingFactory, L"D:\\123.png", imageLayoutRect.right - imageLayoutRect.left,
			imageLayoutRect.bottom - imageLayoutRect.top, &pBitmap) == S_OK)
		{
			m_pRenderTarget->DrawBitmap(pBitmap, imageLayoutRect);

			SafeRelease(&pBitmap);
		}

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
