#pragma once

#include <Windows.h>

// "sz_Or_Ord" (or "szOrOrd") is a placeholder declaration for a variable-length array of 16-bit elements
// to represent what can be an ordinal value or a zero-terminated wide-character string.
// Its precise usage varies according to the structure and member. See linked documentation.
// Because offsets of the next members are not fixed, it's not possible to define a struct
// with members following an sz_Or_Ord. Structure declarations in this file use "WORD name[1]"
// to give the first sz_Or_Ord an offset.
//
// None of these structures is defined in any standard header file.

/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgtemplateex
/// </summary>
//typedef struct {
//	WORD      dlgVer;
//	WORD      signature;
//	DWORD     helpID;
//	DWORD     exStyle;
//	DWORD     style;
//	WORD      cDlgItems;
//	short     x;
//	short     y;
//	short     cx;
//	short     cy;
//	sz_Or_Ord menu;
//	sz_Or_Ord windowClass;
//	WCHAR     title[titleLen];
//	WORD      pointsize;
//	WORD      weight;
//	BYTE      italic;
//	BYTE      charset;
//	WCHAR     typeface[stringLen];
//} DLGTEMPLATEEX;

/// <summary>
/// Declare the declarable part of the DLGTEMPLATEEX structure.
/// Once it gets to the "menu" member, it's no longer a declarable structure.
/// </summary>
typedef struct {
	WORD      dlgVer;
	WORD      signature;
	DWORD     helpID;
	DWORD     exStyle;
	DWORD     style;
	WORD      cDlgItems;
	short     x;
	short     y;
	short     cx;
	short     cy;
	WORD      menu[1];
} DLGTEMPLATEEX_1;


/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgitemtemplateex
/// </summary>
//typedef struct {
//	DWORD     helpID;
//	DWORD     exStyle;
//	DWORD     style;
//	short     x;
//	short     y;
//	short     cx;
//	short     cy;
//	DWORD     id;
//	sz_Or_Ord windowClass;
//	sz_Or_Ord title;
//	WORD      extraCount;
//} DLGITEMTEMPLATEEX;

/// <summary>
/// Declare the declarable part of the DLGITEMTEMPLATEEX structure.
/// Once it gets to the "windowClass" member, it's no longer a declarable structure.
/// </summary>
typedef struct {
	DWORD     helpID;
	DWORD     exStyle;
	DWORD     style;
	short     x;
	short     y;
	short     cx;
	short     cy;
	DWORD     id;
	WORD windowClass[1];
} DLGITEMTEMPLATEEX_1;

#pragma pack (push, 2)
/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/menurc/menuheader
/// </summary>
typedef struct {
	WORD wVersion;
	WORD cbHeaderSize;
} MENUHEADER;
#pragma pack (pop)

///// <summary>
///// https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-menuitemtemplate
///// </summary>
//typedef struct {
//	WORD  mtOption;
//	WORD  mtID;
//	WCHAR mtString[1];
//} MENUITEMTEMPLATE, * PMENUITEMTEMPLATE;

/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/menurc/normalmenuitem
/// Note that menuText is actually an szOrOrd
/// </summary>
typedef struct {
	WORD    resInfo;
	WORD    menuText[1];
} NORMALMENUITEM;


/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/menurc/popupmenuitem
/// Note that menuText is actually an szOrOrd
/// </summary>
typedef struct {
	DWORD   type;
	DWORD   state;
	DWORD   id;
	WORD    resInfo;
	WORD    menuText[1];
} POPUPMENUITEM;

/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/menurc/menuex-template-header
/// </summary>
typedef struct {
	WORD  wVersion;
	WORD  wOffset;
	DWORD dwHelpId;
} MENUEX_TEMPLATE_HEADER;


#pragma pack (push, 2)
/// <summary>
/// https://learn.microsoft.com/en-us/windows/win32/menurc/menuex-template-item
/// </summary>
typedef struct {
	DWORD dwType;
	DWORD dwState;
	//UINT  uId;
	DWORD uId;
	WORD  wFlags;
	WCHAR szText[1];
} MENUEX_TEMPLATE_ITEM;
#pragma pack (pop)