#pragma once
#include "stdafx.h"

#ifndef DECIAL_UTIL_H
#define DECIAL_UTIL_H
#define MATH_PI				(3.1415926f)
#define MAXIMUM_CAPACITY	(1<<30)
#include "miniz.h"

//大于iCap且是2的次幂
INT TableSizeFor(INT iCap) {
	iCap -= 1;
	iCap |= ((unsigned int)iCap) >> 1;
	iCap |= ((unsigned int)iCap) >> 2;
	iCap |= ((unsigned int)iCap) >> 4;
	iCap |= ((unsigned int)iCap) >> 8;
	iCap |= ((unsigned int)iCap) >> 16;
	return (iCap < 0) ? 1 : (iCap >= MAXIMUM_CAPACITY) ? MAXIMUM_CAPACITY : iCap + 1;
}

INT Char2WChar(const char* cchar, WCHAR *wchar) {
	INT ret = 0;
	ret = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), wchar, ret);
	wchar[ret] = '\0';
	return ret;
}

INT WChar2Char(CONST WCHAR *wchar, CHAR *cchar) {
	INT ret = 0;
	ret = WideCharToMultiByte(CP_ACP, 0, wchar, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, wchar, -1, cchar, ret, NULL, NULL);
	return ret;
}

INT LinearEaseIn(INT t, INT iBegin, INT iEnd, INT iDulay) {
	return iEnd * t / iDulay + iBegin;
}

INT LoadResourceFromZip(mz_zip_archive *pZip, const char* pName, LPVOID *pBuff) {
	INT iIndex = 0, iSize = 0;
	if ((iIndex = mz_zip_reader_locate_file(pZip, pName, NULL, 0)) >= 0) {
		mz_zip_archive_file_stat mzafs = { 0 };
		if (mz_zip_reader_file_stat(pZip, iIndex, &mzafs)) {
			LPVOID pTmp = NULL;
			if ((pTmp = mz_zip_reader_extract_to_heap(pZip, iIndex, &iSize, 0)) != NULL) {
				if (*pBuff == NULL) {
					*pBuff = malloc(iSize);
				}
				memcpy(*pBuff, pTmp, iSize);
				pZip->m_pFree(pZip->m_pAlloc_opaque, pTmp);
			}
		}
	}
	return iSize;
}

/**
	获取资源文件内容
*/
DWORD LoadResourceFromRes(HINSTANCE hInstace, int resId, LPVOID* outBuff, LPWSTR resType) {
	HRSRC hRsrc = FindResource(hInstace, MAKEINTRESOURCE(resId), resType);
	if (hRsrc != NULL) {
		DWORD dwSize = SizeofResource(hInstace, hRsrc);
		if (dwSize) {
			HGLOBAL hGlobal = LoadResource(hInstace, hRsrc);
			if (hGlobal != NULL) {
				LPVOID pBuffer = LockResource(hGlobal);
				if (pBuffer != NULL) {
					if (*outBuff == NULL) {
						*outBuff = malloc(dwSize);
						memset(*outBuff, 0, dwSize);
					}
					memcpy(*outBuff, pBuffer, dwSize);
				}
				FreeResource(hGlobal);
			}
			return dwSize;
		}
	}
	return 0;
}

//BGRA到RGBA(截图时需要需要设置FALSE,bTranslate)
VOID BGRA2RGBA(CONST UINT* pIn, UINT* pOut, INT iW, INT iH, BOOL bTranslate) {
	LPBYTE pTmpOut = pOut;
	LPBYTE pTmpIn = pIn;
	INT iWH = iW * iH;
	for (int i = 0; i < iWH; i++) {
		pTmpOut[i * 4 + 3] = bTranslate ? pTmpIn[i * 4 + 3] : 0xFF;
		if (pTmpOut[i * 4 + 3] < 255)
		{
			pTmpOut[i * 4] = (BYTE)((DWORD)(pTmpIn[i * 4 + 2] * pTmpIn[i * 4 + 3]) / 255);
			pTmpOut[i * 4 + 1] = (BYTE)((DWORD)(pTmpIn[i * 4 + 1] * pTmpIn[i * 4 + 3]) / 255);
			pTmpOut[i * 4 + 2] = (BYTE)((DWORD)(pTmpIn[i * 4] * pTmpIn[i * 4 + 3]) / 255);
		}
		else {
			pTmpOut[i * 4] = pTmpIn[i * 4 + 2];
			pTmpOut[i * 4 + 1] = pTmpIn[i * 4 + 1];
			pTmpOut[i * 4 + 2] = pTmpIn[i * 4];
		}
	}
	/*UINT *pI = pIn, *pO = pOut;
	UINT8 *pIVal = NULL, *pOVal = NULL;
	INT iSize = iW * iH;
	while (iSize > 0) {
		pIVal = pI;
		pOVal = pO;
		//R B
		*pOVal = *(pIVal + 2);
		//G G
		*(pOVal + 1) = *(pIVal + 1);
		//B R
		*(pOVal + 2) = *pIVal;
		//A A 不透明
		*(pOVal + 3) = *(pIVal + 3);
		pI++;
		pO++;
		iSize--;
	}*/
}

COLORREF PixelAlpha(COLORREF clrSrc, double src_darken, COLORREF clrDest, double dest_darken) {
	return RGB(
		GetRValue(clrSrc) * src_darken + GetRValue(clrDest) * dest_darken,
		GetGValue(clrSrc) * src_darken + GetGValue(clrDest) * dest_darken,
		GetBValue(clrSrc) * src_darken + GetBValue(clrDest) * dest_darken
	);
}

BOOL WINAPI AlphaBitBlt(HDC hDC, int nDestX, int nDestY, int dwWidth, int dwHeight, HDC hSrcDC, int nSrcX, int nSrcY, int wSrc, int hSrc, BLENDFUNCTION ftn) {
	HDC hTempDC = CreateCompatibleDC(hDC);
	if (NULL == hTempDC)
		return FALSE;

	//Creates Source DIB
	LPBITMAPINFO lpbiSrc = NULL;
	// Fill in the BITMAPINFOHEADER
	lpbiSrc = (LPBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER));
	if (lpbiSrc == NULL) {
		DeleteDC(hTempDC);
		return FALSE;
	}
	lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiSrc->bmiHeader.biWidth = dwWidth;
	lpbiSrc->bmiHeader.biHeight = dwHeight;
	lpbiSrc->bmiHeader.biPlanes = 1;
	lpbiSrc->bmiHeader.biBitCount = 32;
	lpbiSrc->bmiHeader.biCompression = BI_RGB;
	lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
	lpbiSrc->bmiHeader.biClrUsed = 0;
	lpbiSrc->bmiHeader.biClrImportant = 0;

	COLORREF* pSrcBits = NULL;
	HBITMAP hSrcDib = CreateDIBSection(
		hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
		NULL, NULL);

	if ((NULL == hSrcDib) || (NULL == pSrcBits))
	{
		free(lpbiSrc);
		DeleteDC(hTempDC);
		return FALSE;
	}

	HBITMAP hOldTempBmp = (HBITMAP)SelectObject(hTempDC, hSrcDib);
	StretchBlt(hTempDC, 0, 0, dwWidth, dwHeight, hSrcDC, nSrcX, nSrcY, wSrc, hSrc, SRCCOPY);
	SelectObject(hTempDC, hOldTempBmp);

	//Creates Destination DIB
	LPBITMAPINFO lpbiDest = NULL;
	// Fill in the BITMAPINFOHEADER
	lpbiDest = (LPBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER));
	if (lpbiDest == NULL)
	{
		free(lpbiSrc);
		DeleteObject(hSrcDib);
		DeleteDC(hTempDC);
		return FALSE;
	}

	lpbiDest->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbiDest->bmiHeader.biWidth = dwWidth;
	lpbiDest->bmiHeader.biHeight = dwHeight;
	lpbiDest->bmiHeader.biPlanes = 1;
	lpbiDest->bmiHeader.biBitCount = 32;
	lpbiDest->bmiHeader.biCompression = BI_RGB;
	lpbiDest->bmiHeader.biSizeImage = dwWidth * dwHeight;
	lpbiDest->bmiHeader.biXPelsPerMeter = 0;
	lpbiDest->bmiHeader.biYPelsPerMeter = 0;
	lpbiDest->bmiHeader.biClrUsed = 0;
	lpbiDest->bmiHeader.biClrImportant = 0;

	COLORREF* pDestBits = NULL;
	HBITMAP hDestDib = CreateDIBSection(hDC, lpbiDest, DIB_RGB_COLORS, (void **)&pDestBits, NULL, NULL);

	if ((NULL == hDestDib) || (NULL == pDestBits)) {
		free(lpbiSrc);
		DeleteObject(hSrcDib);
		DeleteDC(hTempDC);
		return FALSE;
	}

	SelectObject(hTempDC, hDestDib);
	BitBlt(hTempDC, 0, 0, dwWidth, dwHeight, hDC, nDestX, nDestY, SRCCOPY);
	SelectObject(hTempDC, hOldTempBmp);

	double src_darken;
	BYTE nAlpha;

	for (int pixel = 0; pixel < dwWidth * dwHeight; pixel++, pSrcBits++, pDestBits++) {
		nAlpha = LOBYTE(*pSrcBits >> 24);
		src_darken = (double)(nAlpha * ftn.SourceConstantAlpha) / 255.0 / 255.0;
		if (src_darken < 0.0) src_darken = 0.0;
		*pDestBits = PixelAlpha(*pSrcBits, src_darken, *pDestBits, 1.0 - src_darken);
	} //for

	SelectObject(hTempDC, hDestDib);
	BitBlt(hDC, nDestX, nDestY, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
	SelectObject(hTempDC, hOldTempBmp);

	free(lpbiDest);
	DeleteObject(hDestDib);

	free(lpbiSrc);
	DeleteObject(hSrcDib);

	DeleteDC(hTempDC);
	return TRUE;
}
#endif
