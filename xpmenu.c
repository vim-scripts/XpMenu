/*
 cl /LD xpmenu.c
*/
/* vi:set ts=8 sts=4 sw=4: */
#ifdef _WIN32
# define DLL_EXPORT _declspec(dllexport)
# include <windows.h>
# include <stdio.h>
# pragma comment(lib, "user32.lib")
# pragma comment(lib, "gdi32.lib")
# pragma comment(lib, "msimg32.lib")
#else
# include <dlfcn.h>
# define DLL_EXPORT
# define HMODULE void*
# define LoadLibrary(x) dlopen(x, RTLD_LAZY)
# define FreeLibrary(x) dlclose(x)
# define GetProcAddress(x, y) dlsym(x, (const char *)y)
# ifndef NULL
#  define NULL ((void*)0)
# endif
#endif

#pragma data_seg(".shared")
WNDPROC	pOldWndProc = NULL;
COLORREF crLeftBack, crRightBack, crFocusBack, crGrayBack, crFrameBack;
COLORREF crFocusFore;
HFONT hMenuFont = NULL;
#pragma data_seg()

	DLL_EXPORT char*
dllload(char *pcmd)
{
    static char    retval[16];
    HMODULE hLib;
    hLib = LoadLibrary(pcmd);
    sprintf(retval, "%d", hLib);
    return retval;
}

	DLL_EXPORT char*
dllfree(char *pcmd)
{
    HMODULE hLib;
    hLib = (HMODULE)atol(pcmd);
    FreeLibrary(hLib);
    return NULL;
}

	DLL_EXPORT char*
callfunc(char *pcmd)
{
    char	*plib, *pfunc, *parg;
    typedef char* (*deffunc)(char *);
    deffunc funcproc;
    HMODULE hLib;
    plib = pcmd;
    pfunc = strchr(plib, ',');
    if (!pfunc)
    return NULL;
    *pfunc++ = 0;
    parg = strchr(pfunc, ',');
    if (parg)
    *parg++ = 0;

    hLib = (HMODULE)atol(plib);
    funcproc = (deffunc)GetProcAddress(hLib, pfunc);
    if (funcproc)
    return funcproc(parg);
    return NULL;
}

BOOL CALLBACK FindWindowProc(HWND hwnd, LPARAM lParam)
{
    HWND* pphWnd = (HWND*)lParam;

    if (GetParent(hwnd))
    {
	*pphWnd = NULL;
	return TRUE;
    }
    *pphWnd = hwnd;
    return FALSE;
}

HFONT GetMenuFont(LPCTSTR pFace, INT iSize)
{	 
    NONCLIENTMETRICS nm;
    HFONT hFont;

    nm.cbSize = sizeof(nm);
    if (SystemParametersInfo(
	SPI_GETNONCLIENTMETRICS,
	sizeof(NONCLIENTMETRICS),
	&nm,
	0))
    {
	hFont = CreateFont(
	    //iSize == 0 ? nm.lfMenuFont.lfHeight : -iSize,
	    0,
	    //iSize == 0 ? nm.lfMenuFont.lfWidth : 0,
	    0,
	    0,
	    0,
	    FW_REGULAR,
	    FALSE,
	    FALSE,
	    FALSE,
	    SHIFTJIS_CHARSET,
	    OUT_DEFAULT_PRECIS,
	    CLIP_DEFAULT_PRECIS,
	    PROOF_QUALITY,
	    FF_DONTCARE,
	    pFace);
    }
    return hFont;
}

void SetOwnerDraw(HMENU hMenu, BOOL bMode)
{
    int	id, mnu;
    MENUITEMINFO mii = { sizeof(MENUITEMINFO),MIIM_CHECKMARKS|MIIM_TYPE|MIIM_DATA };
    CHAR    szString[256];
    HMENU hSubMenu;

    if (hMenu == NULL)
	return;
    for(mnu = 0; mnu < GetMenuItemCount(hMenu); mnu++)
    {
	GetMenuItemInfo((HMENU)hMenu, mnu, TRUE, &mii);
	if (bMode)
	{
	    mii.fType |= MFT_OWNERDRAW;
	    if (mii.hSubMenu == NULL && (mii.fType & MF_SEPARATOR) == 0
		    && mii.dwItemData == 0)
	    {
		LPSTR	pCopy;
		GetMenuString(hMenu, mnu, szString, sizeof(szString), MF_BYPOSITION);
		pCopy = strdup(szString);
		if (pCopy)
		{
		    mii.fMask |= MIIM_DATA;
		    mii.dwItemData = (DWORD)pCopy;
		}
	    }
	}
	else
	{
	    mii.fType &= ~MFT_OWNERDRAW;
	    if (mii.hSubMenu == NULL && (mii.fType & MF_SEPARATOR) == 0)
	    {
		mii.fMask &= ~MIIM_DATA;
		if (mii.dwItemData)
		{
		    free((LPSTR)mii.dwItemData);
		    mii.dwItemData = 0;
		}
	    }
	}

	SetMenuItemInfo(hMenu, mnu, TRUE, &mii);

	hSubMenu = GetSubMenu(hMenu, mnu);
	if (hSubMenu)
	    SetOwnerDraw(hSubMenu, bMode);
    }
}

LRESULT CALLBACK
MenuWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int id, cnt, mnu;
    PAINTSTRUCT ps;
    CHAR    szString[256], *pszString;
    HDC	hDC;
    RECT rc;
    POINT pt;
    HFONT hFont, hFontOld;
    HBRUSH hBrush, hBrushOld;
    LPMEASUREITEMSTRUCT lpMI = NULL;
    LPDRAWITEMSTRUCT lpDI = NULL;
    SIZE size;
    MENUITEMINFO mii = { sizeof(MENUITEMINFO),MIIM_CHECKMARKS|MIIM_TYPE|MIIM_SUBMENU };
    NONCLIENTMETRICS nm;
    HMENU hMenu, hMenuTop;
    BOOL isMenuBar;
    static BOOL bFirst = TRUE;

    if (pOldWndProc == NULL)
	return DefWindowProc(hWnd, uMsg, wParam, lParam);

    hMenu = GetMenu(hWnd);

    if (bFirst == TRUE)
    {
	SetOwnerDraw(hMenu, TRUE);
	bFirst = FALSE;
    }

    switch(uMsg)
    {
    case WM_INITMENUPOPUP:
	if (HIWORD(lParam) == 0)
	    SetOwnerDraw((HMENU)wParam, TRUE);
	break;
    case WM_MEASUREITEM:
	lpMI = (LPMEASUREITEMSTRUCT)lParam;
	GetMenuItemInfo((HMENU)hMenu, lpMI->itemID, FALSE, &mii);
	GetMenuString(hMenu, lpMI->itemID, szString, sizeof(szString), MF_BYCOMMAND);
	if (strlen(szString) == 0 && lpMI->itemData)
	    strcpy(szString, (char*)lpMI->itemData);

	if ((mii.fType & MF_SEPARATOR) != 0 ||
	    ((mii.fType & MF_BITMAP) == 0 && strlen(szString) == 0))
	{
	    lpMI->itemHeight = GetSystemMetrics(SM_CYMENU) >> 1;
	    lpMI->itemWidth = 0;
	}
	else
	if (mii.fType & MF_BITMAP)
	{
	    BITMAP bmp_info;
	    GetObject(mii.hbmpItem, sizeof(BITMAP), &bmp_info);
	    lpMI->itemWidth = bmp_info.bmWidth + 2;
	    lpMI->itemHeight = bmp_info.bmHeight + 2;
	}
	else
	{
	    hDC = GetDC(hWnd);
	    if (hMenuFont != NULL)
	    {
		hFont = hMenuFont;
		hFontOld = (HFONT)SelectObject(hDC, hFont);
	    }
	    else
	    {
		nm.cbSize = sizeof(nm);
		if (SystemParametersInfo(
			SPI_GETNONCLIENTMETRICS,
			sizeof(NONCLIENTMETRICS),
			&nm,
			0))
		{
		    hFont = CreateFontIndirect(&nm.lfMenuFont);
		    hFontOld = (HFONT)SelectObject(hDC, hFont);
		}
		else
		    hFont = NULL;
	    }

	    ZeroMemory(&size, sizeof(size));
	    GetTextExtentPoint32(hDC, szString, strlen(szString), &size);

	    if (hFont)
		SelectObject(hDC, hFontOld);

	    if (hFont && hFont != hMenuFont)
		DeleteObject(hFont);

	    ReleaseDC(hWnd, hDC);

	    lpMI->itemWidth = size.cx - GetSystemMetrics(SM_CXEDGE) * 4 + 1;
	    lpMI->itemHeight = size.cy + GetSystemMetrics(SM_CXEDGE) * 2 + 1;

	    if (strlen(szString) == 0)
		lpMI->itemHeight = size.cy + 2;
	    else
	    if (hMenuFont == NULL)
		lpMI->itemHeight = nm.iMenuHeight;
	}
	break;
    case WM_DRAWITEM:
	lpDI = (LPDRAWITEMSTRUCT)lParam;
	if (lpDI->CtlType != ODT_MENU)
	    break;
	GetMenuItemInfo((HMENU)hMenu, lpDI->itemID, FALSE, &mii);
	GetMenuString(hMenu, lpDI->itemID, szString, sizeof(szString), MF_BYCOMMAND);
	if (strlen(szString) == 0 && lpDI->itemData)
	    strcpy(szString, (LPSTR)lpDI->itemData);

	hDC = lpDI->hDC;
	rc = lpDI->rcItem;

	isMenuBar = FALSE;
	for(mnu = 0; mnu < GetMenuItemCount(hMenu); mnu++)
	{
	    if (mii.hSubMenu != NULL && GetSubMenu(hMenu, mnu) == mii.hSubMenu)
	    {
		isMenuBar = TRUE;
		break;
	    }
	}

	if (isMenuBar)
	{
	    hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
	    FillRect(hDC, &rc, hBrush);
	    DeleteObject(hBrush);
	}
	else
	{
	    TRIVERTEX	vert[2];
	    GRADIENT_RECT    grect;

	    hBrush = CreateSolidBrush(crRightBack);
	    FillRect(hDC, &rc, hBrush);
	    DeleteObject(hBrush);

	    vert[0].x      = rc.left;
	    vert[0].y      = rc.top;
	    vert[0].Red    = GetRValue( crLeftBack ) << 8;
	    vert[0].Green  = GetGValue( crLeftBack ) << 8;
	    vert[0].Blue   = GetBValue( crLeftBack ) << 8;
	    vert[0].Alpha  = 0x0000;

	    vert[1].x      = (rc.right - rc.left) / 2;
	    vert[1].y      = rc.bottom; 
	    vert[1].Red    = GetRValue( crRightBack ) << 8;
	    vert[1].Green  = GetGValue( crRightBack ) << 8;
	    vert[1].Blue   = GetBValue( crRightBack ) << 8;
	    vert[1].Alpha  = 0x0000;

	    grect.UpperLeft  = 0;
	    grect.LowerRight = 1;

	    GradientFill(hDC, vert, 2, &grect, 1, GRADIENT_FILL_RECT_H);
	}

	if ((mii.fType & MF_SEPARATOR) != 0 ||
	    ((mii.fType & MF_BITMAP) == 0 && strlen(szString) == 0))
	{
	    hBrush = CreateSolidBrush(crRightBack);
	    SetRect(&rc, rc.left, rc.top+2, rc.right, rc.bottom-2);
	    DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_TOP | BF_BOTTOM);
	    DeleteObject(hBrush);
	}
	else
	{
	    if (lpDI->itemState & ODS_SELECTED &&
		    (lpDI->itemState & ODS_GRAYED) == 0)
	    {
		InflateRect(&rc, -1, -1);
		hBrush = CreateSolidBrush(crFrameBack);
		FrameRect(hDC, &rc, hBrush);
		DeleteObject(hBrush);
		InflateRect(&rc, -1, -1);
		hBrush = CreateSolidBrush(crFocusBack);
		FillRect(hDC, &rc, hBrush);
		DeleteObject(hBrush);
		InflateRect(&rc, 2, 2);
	    }

	    if (mii.fType & MF_BITMAP)
	    {
		BITMAP bmp_info;
		int x;
		COLORREF crTrans;
		HDC hMem = CreateCompatibleDC(hDC);
		GetObject(mii.hbmpItem, sizeof(BITMAP), &bmp_info);
                SelectObject(hMem, mii.hbmpItem);
		crTrans = GetPixel(hMem, 0, 0);
		x = (rc.right - rc.left - bmp_info.bmWidth) / 2;
		TransparentBlt(
			hDC,
			x, 0,
			bmp_info.bmWidth, bmp_info.bmHeight,
			hMem,
			0, 0,
			bmp_info.bmWidth, bmp_info.bmHeight,
			crTrans);
                DeleteDC(hMem);
	    }
	    else
	    {
		COLORREF crOld;

		if (hMenuFont != NULL)
		{
		    hFont = hMenuFont;
		    hFontOld = (HFONT)SelectObject(hDC, hFont);
		}
		else
		{
		    nm.cbSize = sizeof(nm);
		    if (SystemParametersInfo(
			    SPI_GETNONCLIENTMETRICS,
			    sizeof(NONCLIENTMETRICS),
			    &nm,
			    0))
		    {
			hFont = CreateFontIndirect(&nm.lfMenuFont);
			hFontOld = (HFONT)SelectObject(hDC, hFont);
		    }
		    else
			hFont = NULL;
		}

		if (lpDI->itemState & ODS_GRAYED)
		    crOld = SetTextColor(hDC, crGrayBack);
		else
		if (lpDI->itemState & ODS_SELECTED &&
			(lpDI->itemState & ODS_GRAYED) == 0)
		    crOld = SetTextColor(hDC, crFocusFore);

		SetBkMode(hDC, TRUE);

		InflateRect(&rc, -6, -1);

		pszString = strchr(szString, '\t');
		if (pszString)
		{
		    *pszString = 0;
		    DrawText(hDC, szString, strlen(szString), &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
		    pszString++;
		    DrawText(hDC, pszString, strlen(pszString), &rc, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
		}
		else
		    DrawText(hDC, szString, strlen(szString), &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

		if (hFont)
		    SelectObject(hDC, hFontOld);

		if (hFont && hFont != hMenuFont)
		    DeleteObject(hFont);

		if (lpDI->itemState & ODS_GRAYED)
		    SelectObject(hDC, &crOld);
	    }
	}
	break;
    }

    return CallWindowProc(pOldWndProc, hWnd, uMsg, wParam, lParam);
}

CHAR _declspec(dllexport) *EnableXpMenu(CHAR *pEnable)
{
    HWND hTop = NULL;
    HWND hWnd = NULL;
    DWORD dwThreadID;
    static LONG lngProc = 0;

    dwThreadID = GetCurrentThreadId();
    EnumThreadWindows(dwThreadID, FindWindowProc, (LPARAM)&hTop);

    crLeftBack = RGB(200, 200, 200);
    crRightBack = RGB(255, 255, 255);
    crFocusBack = RGB(200, 200, 255);
    crGrayBack = RGB(100, 100, 100);
    crFrameBack = RGB(0, 0, 100);
    if (hMenuFont)
	DeleteObject(hMenuFont);
    hMenuFont = NULL;

    /* is this a vim ? */
    hWnd = FindWindowEx(hTop, NULL, "VimTextArea", NULL);
    if (hWnd)
    {
	if (!pEnable || strlen(pEnable) == 0)
	{
	    //SetOwnerDraw(GetMenu(hTop), FALSE);
	    //if (pOldWndProc)
		//SetWindowLong(hTop, GWL_WNDPROC, (LONG)pOldWndProc);
	    //pOldWndProc = NULL;
	    // disable don't work [FIXME]
	}
	else
	{
	    if (!pOldWndProc)
	    {
		pOldWndProc = (WNDPROC)GetWindowLong(hTop, GWL_WNDPROC);
		SetWindowLong(hTop, GWL_WNDPROC, (LONG)MenuWndProc);
	    }
	}
    }
    return NULL;
}

CHAR _declspec(dllexport) *SetLeftBack(CHAR *pRGB)
{
    if (!pRGB || strlen(pRGB) == 0)
    {
	crLeftBack = RGB(255, 255, 255);
    }
    else
    {
	int r, g, b;
	sscanf(pRGB, "%d,%d,%d", &r, &g, &b);
	crLeftBack = RGB(r, g, b);
    }
    return NULL;
}

CHAR _declspec(dllexport) *SetRightBack(CHAR *pRGB)
{
    if (!pRGB || strlen(pRGB) == 0)
    {
	crRightBack = RGB(255, 255, 255);
    }
    else
    {
	int r, g, b;
	sscanf(pRGB, "%d,%d,%d", &r, &g, &b);
	crRightBack = RGB(r, g, b);
    }
    return NULL;
}

CHAR _declspec(dllexport) *SetFocusBack(CHAR *pRGB)
{
    if (!pRGB || strlen(pRGB) == 0)
    {
	crFocusBack = RGB(200, 200, 255);
    }
    else
    {
	int r, g, b;
	sscanf(pRGB, "%d,%d,%d", &r, &g, &b);
	crFocusBack = RGB(r, g, b);
    }
    return NULL;
}

CHAR _declspec(dllexport) *SetFocusFore(CHAR *pRGB)
{
    if (!pRGB || strlen(pRGB) == 0)
    {
	crFocusFore = RGB(0, 0, 0);
    }
    else
    {
	int r, g, b;
	sscanf(pRGB, "%d,%d,%d", &r, &g, &b);
	crFocusFore = RGB(r, g, b);
    }
    return NULL;
}

CHAR _declspec(dllexport) *SetFrameLine(CHAR *pRGB)
{
    if (!pRGB || strlen(pRGB) == 0)
    {
	crFrameBack = RGB(0, 0, 100);
    }
    else
    {
	int r, g, b;
	sscanf(pRGB, "%d,%d,%d", &r, &g, &b);
	crFrameBack = RGB(r, g, b);
    }
    return NULL;
}

CHAR _declspec(dllexport) *SetFont(CHAR *pFont)
{
    if (pFont && strlen(pFont))
    {
	LPSTR pCopy = strdup(pFont);
	if (pCopy)
	{
	    INT	iSize = 0;
	    LPSTR pSize = strstr(pCopy, ":h");
	    if (pSize)
	    {
		*pSize = 0;
		iSize = atol(pSize + 2);
	    }

	    if (hMenuFont)
		DeleteObject(hMenuFont);
	    hMenuFont = GetMenuFont(pCopy, iSize);
	    if (pCopy)
		free(pCopy);
	}
    }
    else
    {
	if (hMenuFont)
	    DeleteObject(hMenuFont);
	hMenuFont = NULL;
    }
    return NULL;
}
