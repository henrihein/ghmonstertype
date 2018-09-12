#pragma once

#define ROPPDSao            (DWORD)0x00F802E5 /* PDSao   */
#define	ROPDPSao     		(DWORD)0x00EA02E9 /* DPSao  */
#define	ROPDSna     		(DWORD)0x00220326 /* DSna  */


LONG SizeFromBitmap(const HBITMAP hbm, const LONG defSize);
LONG GetBitmapRes(const HBITMAP hbm);
bool MaskToTransparent(HBITMAP hbmSrc, HDC hdcDst, COLORREF colormask, const LONG imgSize);
bool SaveWithTransparent(const wchar_t pathname[], HBITMAP hbmSrc, COLORREF colormask, const LONG imgSize);

class CGDIInit
{
public:
	CGDIInit();
	virtual ~CGDIInit();

private:
	Gdiplus::GdiplusStartupInput	m_startupInput;
	ULONG_PTR						m_token;
};

class CDCScreen
{
public:
	CDCScreen(void);
	virtual ~CDCScreen(void);

	operator HDC() const { return m_hdc; }

private:
	CDCScreen(const CDCScreen &);
	HDC m_hdc;
};


class CDCMem
{
public:
	CDCMem(void);
	virtual ~CDCMem(void);

	operator HDC() const { return m_hdc; }

	void FillWith(COLORREF color, LONG imgSize);
	void FillTransparent(LONG imgSize);

private:
	CDCMem(const CDCMem &);
	HDC m_hdc;
};

class CBitmapMem
{
public:
	CBitmapMem(unsigned int imgSize = 499);
	virtual ~CBitmapMem();

	operator HBITMAP() const { return m_hbm; }

private:
	CBitmapMem(const CBitmapMem &);
	HBITMAP m_hbm;
};

class CBmSelector
{
public:
	CBmSelector(HDC hdc, HBITMAP hbm);
	virtual ~CBmSelector();

	bool operator()() const { return NULL != m_hbmSav; }

private:
	CBmSelector(const CBmSelector &);
	HGDIOBJ m_hbmSav;
	const HDC m_hdc;
};

class CSolidBrushSelector
{
public:
	CSolidBrushSelector(COLORREF color, HDC hdc);
	virtual ~CSolidBrushSelector();

	operator HBRUSH() const			{ return m_hbr; }
	bool operator ()() const		{ return NULL != m_hbr; }

private:
	const HDC m_hdc;
	HBRUSH m_hbr;
	HGDIOBJ m_hbrSav;
};
