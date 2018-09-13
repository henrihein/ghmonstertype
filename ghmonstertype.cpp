// ghmonstertype.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GdiHelpers.h"
#include "resource.h"

const COLORREF color_bad		= RGB(1, 2, 3);
const COLORREF color_black		= RGB(0, 0, 0);
const COLORREF color_white		= RGB(255, 255, 255);
const COLORREF color_elite		= RGB(237, 200, 27);
const COLORREF color_normal		= RGB(248, 248, 248);
const COLORREF color_none		= RGB(0, 0, 0);
const COLORREF color_mask		= RGB(0, 128, 128);

typedef enum MONSTER_ORIENTATION {
	MO_UNKNOWN = -1,
	MO_HORIZONTAL,
	MO_VERTICAL
};


typedef struct tagMonsterTypeData
{
	MONSTER_ORIENTATION		m_orientation;
	COLORREF				m_type2, m_type3, m_type4;

} MonsterTypeData;

int OnUsage(const wchar_t *trigger = NULL)
{
	int ret = 0;

	if (trigger)
	{
		wprintf(L"Confused by: %s\r\n", trigger);
		ret = 1;
	}
	printf("Usage: ghmonstertype [Type2] [Type3] [Type4] [orientation] [filename] [size]\r\n");
	printf("\tType#: [none|normal|elite]\r\n");
	printf("\tOrientation: [vertical|horizontal]\r\n");
	printf("\tSize is optional. Default is 500x500.\r\n");
	return ret;
}

COLORREF TypeFromArg(wchar_t *arg)
{
	if (0 == _wcsicmp(arg, L"none")) return color_black;
	if (0 == _wcsicmp(arg, L"nothing")) return color_black;	//common synonym
	if (0 == _wcsicmp(arg, L"normal")) return color_normal;
	if (0 == _wcsicmp(arg, L"regular")) return color_normal;
	if (0 == _wcsicmp(arg, L"elite")) return color_elite;
	return color_bad;
}

int SaveMonsterTypeImage(const wchar_t *filename, HBITMAP hbm, const LONG imgSize)
{
	wchar_t pathname[MAX_PATH];
	wchar_t *ppath = NULL;
	bool isRelativePath = true;

	//On Windows, there isn't a good way to tell if a string is an absolute or relative path.
	//Checking the most common/naive ways here.  
	if ((NULL != wcschr(filename, '\\')) && (2 <= wcslen(filename)))
	{
		if ('\\' == filename[0])
			isRelativePath = false;
		else if ((':' == filename[1]) && ('\\' == filename[2]))
			isRelativePath = false;
		else if (('\\' == filename[0]) && ('\\' == filename[1]))
			isRelativePath = false;
		else if (('.' == filename[0]) && ('\\' == filename[1]))
			isRelativePath = false;
	}
	if (isRelativePath)
	{
		wchar_t *wszLastPath = NULL;

		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &ppath)))
		{
			StringCbCopyW(pathname, MAX_PATH, ppath);
			CoTaskMemFree(ppath);
		}
		else
			GetCurrentDirectoryW(MAX_PATH, pathname);
		StringCbCatW(pathname, MAX_PATH, L"\\");
		StringCbCatW(pathname, MAX_PATH, filename);
		//See if we need to create the path
		wszLastPath = wcsrchr(pathname, '\\');
		assert(wszLastPath);
		if (wszLastPath)
		{
			wszLastPath[0] = '\0';
			if (!PathIsDirectoryW(pathname))
			{
				if (!CreateDirectoryW(pathname, NULL))
				{
					wprintf(L"Could not create directory %s\r\n", pathname);
					return 17;
				}
			}
			wszLastPath[0] = '\\';
		}
	}
	else
		StringCbCopyW(pathname, MAX_PATH, filename);
	if (SaveWithTransparent(pathname, hbm, color_mask, imgSize))
		return 0;
	_putws(pathname);
	_putws(L"  -> Saving to file failed.\r\n");
	return 11;
}

//Blt a single crust piece
bool BltCrustPiece(HDC hdcDst, LONG xDst, LONG yDst, LONG dxDst, LONG dyDst, HDC hdcSrc, COLORREF color, LONG xSrc, LONG ySrc, LONG dxSrc, LONG dySrc)
{
	CSolidBrushSelector mtBrush(color, hdcDst);

	//Check if we need one or two blts
	if (color_white == color)
		return (StretchBlt(hdcDst, xDst, yDst, dxDst, dyDst, hdcSrc, xSrc, ySrc, dxSrc, dySrc, SRCPAINT))
				? true : false;
	if (color_black == color)
		return (StretchBlt(hdcDst, xDst, yDst, dxDst, dyDst, hdcSrc, xSrc, ySrc, dxSrc, dySrc, ROPDSna))
				? true : false;
	//For readability, spread out the two blts into separate statements, instead of one expression, as order is important.
	if (StretchBlt(hdcDst, xDst, yDst, dxDst, dyDst, hdcSrc, xSrc, ySrc, dxSrc, dySrc, ROPDSna))
		return (StretchBlt(hdcDst, xDst, yDst, dxDst, dyDst, hdcSrc, xSrc, ySrc, dxSrc, dySrc, ROPDPSao))
				? true : false;
	return false;
}

//Blt the 3 crust pieces
bool BltCrust(HDC hdcDst, HDC hdcSrc, const MonsterTypeData &mtd, LONG imgSizeDst, LONG imgSizeSrc)
{
	return
		BltCrustPiece(hdcDst, 0, 0, imgSizeDst / 2, imgSizeDst / 2, hdcSrc, mtd.m_type2, 0, 0, imgSizeSrc / 2, imgSizeSrc / 2) &&
		BltCrustPiece(hdcDst, imgSizeDst / 2, 0, imgSizeDst / 2, imgSizeDst / 2, hdcSrc, mtd.m_type3, imgSizeSrc / 2, 0, imgSizeSrc / 2, imgSizeSrc / 2) &&
		BltCrustPiece(hdcDst, 0, imgSizeDst / 2, imgSizeDst, imgSizeDst / 2, hdcSrc, mtd.m_type4, 0, imgSizeSrc / 2, imgSizeSrc, imgSizeSrc / 2);
}

int CreateMonsterTypeImage(const wchar_t *filename, const MonsterTypeData &mtd, LONG imgSizePreferred)
{
	CGDIInit gdi;
	const WORD idb = MO_HORIZONTAL == mtd.m_orientation ? IDB_HORIZONTAL : IDB_VERTICAL;
	HBITMAP hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(idb), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	int retOp = 2;

	if (hbm)
	{
		const LONG imgSizeSrc = SizeFromBitmap(hbm, 499);
		const LONG imgSizeDst = imgSizePreferred ? imgSizePreferred : imgSizeSrc;
		CDCMem hdcMem, hdcNew, hdcFinal;
		CBitmapMem hbmNew(imgSizeDst);
		bool save = false;

		if (hdcMem && hbmNew)
		{
			CBmSelector bmSelNew(hdcNew, hbmNew);
			CBmSelector bmSelMem(hdcMem, hbm);

			hdcNew.FillWith(color_mask, imgSizeDst);
			if (bmSelNew() && bmSelMem())
			{
				save = BltCrust(hdcNew, hdcMem, mtd, imgSizeDst, imgSizeSrc);
			}
		}
		if (save)
			retOp = SaveMonsterTypeImage(filename, hbmNew, imgSizeDst);
		DeleteObject(hbm);
		if (0 == retOp)
			wprintf(L"Created %s\r\n", filename);
	}
	return retOp;
}

int wmain(int argc, wchar_t* argv[])
{
	MonsterTypeData mtd;
	wchar_t wszFilename[MAX_PATH];
	LONG imgSize = 0;

	if (1 >= argc)
		return OnUsage();
	if (6 > argc)
		return OnUsage(L"Too few arguments");
	if (color_bad == (mtd.m_type2 = TypeFromArg(argv[1])))
		return OnUsage(argv[1]);
	if (color_bad == (mtd.m_type3 = TypeFromArg(argv[2])))
		return OnUsage(argv[2]);
	if (color_bad == (mtd.m_type4 = TypeFromArg(argv[3])))
		return OnUsage(argv[3]);
	if (0 == _wcsicmp(argv[4], L"vertical"))
		mtd.m_orientation = MO_VERTICAL;
	else if (0 == _wcsicmp(argv[4], L"horizontal"))
		mtd.m_orientation = MO_HORIZONTAL;
	else
		return OnUsage(argv[4]);
	StringCbCopyW(wszFilename, MAX_PATH, argv[5]);
	if (6 == argc)
		imgSize = 500;
	else if (7 == argc)
	{
		imgSize = wcstol(argv[6], NULL, 10);
		if ((0 == imgSize) || (LONG_MAX == imgSize) || (LONG_MIN == imgSize))
			return OnUsage(argv[6]);
	}
	else
		return OnUsage(L"Too many arguments");
	return CreateMonsterTypeImage(wszFilename, mtd, imgSize);
}

