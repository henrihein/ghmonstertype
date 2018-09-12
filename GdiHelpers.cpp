#include "stdafx.h"
#include "GdiHelpers.h"

UINT GetEncoderClsid(const WCHAR* format, CLSID &clsid);
bool MaskToTransparent(Gdiplus::Bitmap &bmSrc, Gdiplus::Graphics &drawer, COLORREF colormask, const LONG imgSize);

LONG SizeFromBitmap(const HBITMAP hbm, const LONG defSize)
{
	assert(hbm);
	if (hbm)
	{
		BITMAP bmData;

		memset(&bmData, 0, sizeof(bmData));
		if (GetObject(hbm, sizeof(bmData), &bmData))
		{
			//Height may be negative
			assert(abs(bmData.bmWidth) == abs(bmData.bmHeight));
			return bmData.bmWidth;
		}
	}
	//Most likely we are trying to load a bum bitmap
	assert(false);
	return defSize;
}
LONG GetBitmapRes(const HBITMAP hbm)
{
	assert(hbm);
	if (hbm)
	{
		BITMAP bmData;

		memset(&bmData, 0, sizeof(bmData));
		if (GetObject(hbm, sizeof(bmData), &bmData))
		{
			//Height may be negative
			assert(abs(bmData.bmWidth) == abs(bmData.bmHeight));
			return bmData.bmBitsPixel;
		}
	}
	//Most likely we are trying to load a bum bitmap
	assert(false);
	return 0;
}
bool MaskToTransparent(Gdiplus::Bitmap &bmSrc, Gdiplus::Graphics &drawer, COLORREF colormask, const LONG imgSize)
{
	Gdiplus::ImageAttributes imageAttributes;
	Gdiplus::ColorMap colorMap = { Gdiplus::Color(GetRValue(colormask), GetGValue(colormask), GetBValue(colormask)),
									Gdiplus::Color(0, 255, 0, 0) };
	const Gdiplus::REAL imgSizeR = static_cast<Gdiplus::REAL>(imgSize);
	const Gdiplus::RectF rect(0, 0, imgSizeR, imgSizeR);

	imageAttributes.SetRemapTable(1, &colorMap, Gdiplus::ColorAdjustTypeBitmap);
	if (Gdiplus::Ok == drawer.DrawImage(&bmSrc, rect, 0, 0, imgSizeR, imgSizeR, Gdiplus::UnitPixel, &imageAttributes))
	{
		return true;
	}
	return false;
}
bool MaskToTransparent(HBITMAP hbmSrc, HDC hdcDst, COLORREF colormask, const LONG imgSize)
{
	Gdiplus::Bitmap bmSrc(hbmSrc, NULL);
	Gdiplus::Graphics drawer(hdcDst);

	return MaskToTransparent(bmSrc, drawer, colormask, imgSize);
}
bool MaskToTransparent(HBITMAP hbmSrc, Gdiplus::Bitmap &bmDst, COLORREF colormask, const LONG imgSize)
{
	Gdiplus::Bitmap bmSrc(hbmSrc, NULL);
	Gdiplus::Graphics drawer(&bmDst);

	return MaskToTransparent(bmSrc, drawer, colormask, imgSize);
}

bool SaveWithTransparent(const wchar_t pathname[], HBITMAP hbmSrc, COLORREF colormask, const LONG imgSize)
{
	Gdiplus::Bitmap bmDst(imgSize, imgSize, PixelFormat32bppPARGB);
	CLSID pngClsid;

	MaskToTransparent(hbmSrc, bmDst, colormask, imgSize);
	if (UINT_MAX == GetEncoderClsid(L"image/png", pngClsid))
		return false;
	return Gdiplus::Ok == bmDst.Save(pathname, &pngClsid, NULL);
}

CGDIInit::CGDIInit()
{
	GdiplusStartup(&m_token, &m_startupInput, NULL);
}
CGDIInit::~CGDIInit()
{
	Gdiplus::GdiplusShutdown(m_token);
}

CDCScreen::CDCScreen(void)
{
	m_hdc = GetDC(NULL);
}
CDCScreen::~CDCScreen(void)
{
	if (m_hdc) ReleaseDC(NULL, m_hdc);
}

CDCMem::CDCMem(void)
{
	m_hdc = CreateCompatibleDC(NULL);
}
CDCMem::~CDCMem(void)
{
	if (m_hdc) DeleteDC(m_hdc);
}
void CDCMem::FillWith(COLORREF color, LONG imgSize)
{
	CSolidBrushSelector brush(color, m_hdc);
	const RECT rc = { 0, 0, imgSize + 1, imgSize + 1 };

	FillRect(m_hdc, &rc, brush);
}
void CDCMem::FillTransparent(LONG imgSize)
{
	Gdiplus::Graphics plane(m_hdc);
	Gdiplus::SolidBrush transparentbrush(Gdiplus::Color(0, 0, 0, 255));

	plane.FillRectangle(&transparentbrush, 0, 0, imgSize + 1, imgSize + 1);
}

CBitmapMem::CBitmapMem(unsigned int imgSize)
{
	CDCScreen hdcScreen;
	BITMAPINFO bmi;
	void *pvBits = NULL;

	memset(&bmi, 0, sizeof(BITMAPINFO));
	if (hdcScreen)	
	{
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = imgSize;
		bmi.bmiHeader.biHeight = imgSize;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;         // four 8-bit components 
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = imgSize * imgSize * 4;
		//Need to use CreateDIBSection to get 32 bit
		m_hbm = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	}
}
CBitmapMem::~CBitmapMem()
{
	if (m_hbm)
		DeleteObject(m_hbm);
}

CBmSelector::CBmSelector(HDC hdc, HBITMAP hbm) : m_hbmSav(NULL), m_hdc(hdc)
{
	if (hdc && hbm)
		m_hbmSav = SelectObject(hdc, hbm);
}
CBmSelector::~CBmSelector()
{
	if (m_hdc && m_hbmSav)
		SelectObject(m_hdc, m_hbmSav);
}

CSolidBrushSelector::CSolidBrushSelector(COLORREF color, HDC hdc) : m_hdc(hdc), m_hbrSav(NULL), m_hbr(NULL)
{
	if (m_hdc)
	{
		if (m_hbr = CreateSolidBrush(color))
			m_hbrSav = SelectObject(m_hdc, m_hbr);
	}
}
CSolidBrushSelector::~CSolidBrushSelector()
{
	if (m_hdc && m_hbrSav)
		SelectObject(m_hdc, m_hbrSav);
	if (m_hbr)
		DeleteObject(m_hbr);
}

UINT GetEncoderClsid(const WCHAR* format, CLSID &clsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	UINT ixEncoderFound = UINT_MAX;
	BYTE *data = NULL;
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (0 == size) return UINT_MAX;
	pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(data = new BYTE[size]);
	if (NULL == pImageCodecInfo) return UINT_MAX;
	GetImageEncoders(num, size, pImageCodecInfo);
	for(UINT ixEncoder = 0; ixEncoder < num; ++ixEncoder)
	{
		if (wcscmp(pImageCodecInfo[ixEncoder].MimeType, format) == 0)
		{
			clsid = pImageCodecInfo[ixEncoder].Clsid;
			ixEncoderFound = ixEncoder;
		}    
	}
	if (data) delete [] data;
	return ixEncoderFound;
	}
