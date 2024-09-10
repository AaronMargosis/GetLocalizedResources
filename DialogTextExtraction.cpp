#include <Windows.h>
#include <iostream>
#include <sstream>
#include "DialogTextExtraction.h"
#include "SysErrorMessage.h"
#include "UtilityFunctions.h"
#include "ResourceDefs.h"

/*
References:

The evolution of dialog templates - 32-bit Classic Templates - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20040621-00/?p=38793

The evolution of dialog templates - 32-bit Extended Templates - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20040623-00/?p=38753

The evolution of dialog templates - Summary - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20040624-00/?p=38733

Resource File Formats - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/menurc/resource-file-formats#dialog-box-resources

DLGTEMPLATE (winuser.h) - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-dlgtemplate

DLGITEMTEMPLATE (winuser.h) - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-dlgitemtemplate

DLGTEMPLATEEX structure - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgtemplateex

DLGITEMTEMPLATEEX structure - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgitemtemplateex
*/

const wchar_t* const sz_Dialog_ = L"[Dialog]";
const wchar_t* const sz_Caption_ = L"[Caption]";

/// <summary>
/// Returns true if the pointed-to resource has the signature of an extended dialog template.
/// </summary>
static bool IsExtendedDialogTemplate(LPVOID pResource)
{
    WORD* pWord = (WORD*)pResource;
    if (*pWord++ != 1)
        return false;
    if (*pWord != 0xffff)
        return false;
    return true;
}

/// <summary>
/// Given the address of a "sz_Or_Ord," return the memory address immediately following it.
/// </summary>
static inline uint16_t* Uint16AfterSzOrOrd(uint16_t* pMem)
{
    uint16_t w0 = *pMem;
    switch (w0)
    {
    case 0x0000:
        return pMem + 1;
    case 0xffff:
        return pMem + 2;
    default:
        while (0 != *pMem++)
            ;
        return pMem;
    }
}

/// <summary>
/// Given the address of a zero-terminated wide-character string, return the memory address immediately following it.
/// </summary>
static inline uint16_t* Uint16AfterSz(uint16_t* pMem)
{
    while (0 != *pMem++)
        ;
    return pMem;
}

static inline std::wstring WindowClassName(const uint16_t* pMem, DWORD dwStyle)
{
    if (0xFFFF == *pMem)
    {
        switch (pMem[1])
        {
        case 0x0080:
            switch (dwStyle & BS_TYPEMASK)
            {
            case BS_3STATE:
            case BS_CHECKBOX:
            case BS_AUTO3STATE:
            case BS_AUTOCHECKBOX:
                return L"Checkbox";
            case BS_RADIOBUTTON:
            case BS_AUTORADIOBUTTON:
                return L"Radio button";
            case BS_GROUPBOX:
                return L"Group box";
            default:
                return L"Button";
            }
        case 0x0081: return L"Edit";
        case 0x0082: return L"Static";
        case 0x0083: return L"List box";
        case 0x0084: return L"Scroll bar";
        case 0x0085: return L"Combo box";
        default: 
            {
            std::wstringstream str;
            str << L"Ordinal " << pMem[1];
            return str.str();
            }
        }
    }
    else
    {
        return (const wchar_t*)pMem;
    }
}

/// <summary>
/// Process an extended dialog template.
/// Output a line of tab-delimited information for non-empty dialog caption and item text.
/// </summary>
/// <param name="lpName"></param>
/// <param name="pResource"></param>
/// <param name="dwResourceSize"></param>
/// <param name="out"></param>
/// <param name="err"></param>
/// <returns></returns>
static bool ProcessExtendedDialogTemplate(LPCWSTR lpName, LPVOID pResource, DWORD dwResourceSize, std::wostream& out, std::wostream& err)
{
    UNREFERENCED_PARAMETER(err);
    //TODO: Need to make sure not to look beyond size of resource.
    UNREFERENCED_PARAMETER(dwResourceSize);

    // Point to the beginning of the dialog template
    DLGTEMPLATEEX_1* pDlgTemplateEx1 = (DLGTEMPLATEEX_1*)pResource;
    WORD nDlgItems = pDlgTemplateEx1->cDlgItems;
    // Point to dialog's window class:
    uint16_t* pMem = Uint16AfterSzOrOrd(pDlgTemplateEx1->menu);
    // Point to dialog's title/caption
    pMem = Uint16AfterSzOrOrd(pMem);
    // Output line if the title/caption is not empty
    if (0x0000 != *pMem)
    {
        std::wstring sText = escapeCrLfTab((const wchar_t*)pMem);
        out
            << RSRCID_t(lpName) << L"\t"
            << sz_Caption_ << L"\t"
            << RemoveAccelsFromText(sText) << L"\t"
            << sText << L"\t"
            << sz_Dialog_
            << std::endl;
    }
    // Point to pointsize, weight, etc. after title
    pMem = Uint16AfterSz(pMem);
    // Skip over pointsize, weight, italic, charset
    pMem += 3;
    if (0 != (pDlgTemplateEx1->style & (DS_SETFONT | DS_SHELLFONT)))
    {
        // skip over typeface
        pMem = Uint16AfterSz(pMem);
    }

    // Dialog items
    for (WORD ixDlgItem = 0; ixDlgItem < nDlgItems; ++ixDlgItem)
    {
        // Each DLGITEMTEMPLATEEX must be aligned on a four-byte boundary
        while (0 != ((unsigned long long)pMem & 0x03))
            pMem++;

        // Get the beginning of the dialog item template:
        DLGITEMTEMPLATEEX_1* pDlgItemEx1 = (DLGITEMTEMPLATEEX_1*)pMem;
        // Point to dialog item's title/text (after its window class)
        pMem = Uint16AfterSzOrOrd(pDlgItemEx1->windowClass);
        // Output a line if it's a zero-terminated string
        if (0x0000 != *pMem && 0xFFFF != *pMem)
        {
            std::wstring sText = escapeCrLfTab((const wchar_t*)pMem);
            out
                << RSRCID_t(lpName) << L"\t"
                << (long)pDlgItemEx1->id << L"\t"
                << RemoveAccelsFromText(sText) << L"\t"
                << sText << L"\t"
                << WindowClassName(pDlgItemEx1->windowClass, pDlgItemEx1->style)
                << std::endl;
        }
        // Get to and through the extraCount
        pMem = Uint16AfterSzOrOrd(pMem);
        WORD cbExtra = *pMem++;
        pMem += (cbExtra / 2);
    }

    return true;
}

/// <summary>
/// Process a standard/"classic" dialog template.
/// Output a line of tab-delimited information for non-empty dialog caption and item text.
/// </summary>
/// <param name="lpName"></param>
/// <param name="pResource"></param>
/// <param name="dwResourceSize"></param>
/// <param name="out"></param>
/// <param name="err"></param>
/// <returns></returns>
static bool ProcessStandardDialogTemplate(LPCWSTR lpName, LPVOID pResource, DWORD dwResourceSize, std::wostream& out, std::wostream& err)
{
    UNREFERENCED_PARAMETER(err);
    //TODO: Need to make sure not to look beyond size of resource.
    UNREFERENCED_PARAMETER(dwResourceSize);

    // Point to the beginning of the dialog template
    DLGTEMPLATE* pDlgTemplate = (DLGTEMPLATE*)pResource;
    WORD nDlgItems = pDlgTemplate->cdit;
    // Point to Menu designation after declared structure
    uint16_t* pMem = (uint16_t*)(pDlgTemplate + 1);
    // Point to dialog's window class
    pMem = Uint16AfterSzOrOrd(pMem);
    // Point to dialog's title/caption
    pMem = Uint16AfterSzOrOrd(pMem);
    // Output line if the title/caption is not empty
    if (0x0000 != *pMem)
    {
        std::wstring sText = escapeCrLfTab((const wchar_t*)pMem);
        out
            << RSRCID_t(lpName) << L"\t"
            << sz_Caption_ << L"\t"
            << RemoveAccelsFromText(sText) << L"\t"
            << sText << L"\t"
            << sz_Dialog_
            << std::endl;
    }
    // Point to memory after title
    pMem = Uint16AfterSz(pMem);
    // if DS_SETFONT is set, move pointer past the font size and name.
    if (0 != (pDlgTemplate->style & DS_SETFONT))
    {
        pMem = Uint16AfterSz(pMem + 1);
    }

    // Dialog items
    for (WORD ixDlgItem = 0; ixDlgItem < nDlgItems; ++ixDlgItem)
    {
        // Each DLGITEMTEMPLATE must be aligned on a four-byte boundary
        while (0 != ((unsigned long long)pMem & 0x03))
            pMem++;

        // Get the beginning of the dialog item template:
        DLGITEMTEMPLATE* pDlgItem = (DLGITEMTEMPLATE*)pMem;
        // Point to the item's window class
        pMem = (uint16_t*)(pDlgItem + 1);
        std::wstring sWindowClassName = WindowClassName(pMem, pDlgItem->style);
        // Point to dialog item's title/text (after its window class)
        pMem = Uint16AfterSzOrOrd(pMem);

        // Output a line if it's a zero-terminated string
        if (0x0000 != *pMem && 0xFFFF != *pMem)
        {
            std::wstring sText = escapeCrLfTab((const wchar_t*)pMem);
            out
                << RSRCID_t(lpName) << L"\t"
                << pDlgItem->id << L"\t"
                << RemoveAccelsFromText(sText) << L"\t"
                << sText << L"\t"
                << sWindowClassName
                << std::endl;
        }

        // Get to and through the extra count / creation data
        pMem = Uint16AfterSzOrOrd(pMem);
        WORD cbExtra = *pMem++;
        pMem += (cbExtra / 2);
    }

    return true;
}

/// <summary>
/// Callback function to handle each dialog resource in the current file.
/// </summary>
/// <param name="hModule">Handle to the resource DLL</param>
/// <param name="lpType">Resource type</param>
/// <param name="lpName">Resource name/identifier</param>
/// <param name="lParam">Pointer to output stream</param>
/// <returns>Always returns TRUE to continue enumeration</returns>
static BOOL CALLBACK EnumDialogCallbackProc(
    _In_opt_ HMODULE hModule,
    _In_ LPCWSTR lpType,
    _In_ LPWSTR lpName,
    _In_ LONG_PTR lParam)
{
    streams_t* pStreams = (streams_t*)lParam;
    // Should enumerate only RT_DIALOGs, but check again just to be safe
    if (RT_DIALOG == lpType)
    {
// Eliminate false positive about lpType possibly being 0
#pragma warning(push)
#pragma warning (disable: 6387)
        HRSRC hRsrc = FindResourceW(hModule, lpName, lpType);
#pragma warning(pop)
        if (NULL != hRsrc)
        {
            DWORD dwResourceSize = SizeofResource(hModule, hRsrc);
            HGLOBAL hGbl = LoadResource(hModule, hRsrc);
            if (NULL != hGbl)
            {
                LPVOID pData = LockResource(hGbl);
                if (IsExtendedDialogTemplate(pData))
                    ProcessExtendedDialogTemplate(lpName, pData, dwResourceSize, pStreams->WCout, pStreams->WCerr);
                else
                    ProcessStandardDialogTemplate(lpName, pData, dwResourceSize, pStreams->WCout, pStreams->WCerr);
            }
        }
    }
    else
    {
        // This should never happen.
        pStreams->WCerr << L"UNEXPECTED RESOURCE TYPE: " << RSRCID_t(lpType) << L", ID " << RSRCID_t(lpName) << std::endl;
    }
    return TRUE;
}


/// <summary>
/// Outputs localized text in the module's dialog resources as tab-delimited fields.
/// Output includes the dialog ID, control ID, the localized text both with accelerators
/// and with accelerator characters removed, and the control type.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool DialogTextExtraction(HMODULE hModule, streams_t& streams)
{
    // Tab-delimited headers
    streams.WCout
        << L"Dialog ID\t"
        << L"Ctrl ID\t"
        << L"Localized text\t"
        << L"Dialog text\t"
        << L"Ctrl Type"
        << std::endl;

    // Enumerate the dialog resources
    if (!EnumResourceNamesW(hModule, RT_DIALOG, EnumDialogCallbackProc, (LPARAM)&streams))
    {
        DWORD dwLastErr = GetLastError();
        streams.WCerr << L"EnumResourceNamesW failed: " << SysErrorMessageWithCode(dwLastErr) << std::endl;
        return false;
    }

    return true;
}

#if 0
// The code below was used to validate the dialog template parsing. It is not being used at this time,
// but I'm keeping it around in case further validation tests are needed.
// Disable warning C4505 so the build can still succeed.
// Warning C4505: unreferenced function with internal linkage has been removed
#pragma warning( disable: 4505 )

static bool ValidateDLGTEMPLATEEX(LPVOID pResource, std::wostream& out, std::wostream& err)
{
    UNREFERENCED_PARAMETER(err);
    // uint16_t -- clearer than WORD
    //TODO: Need to make sure not to look beyond size of resource.
    DLGTEMPLATEEX_1* pDlgTemplateEx1 = (DLGTEMPLATEEX_1*)pResource;
    WORD nDlgItems = pDlgTemplateEx1->cDlgItems;
    out << L"  " << L"Items: " << nDlgItems << std::endl;
    if (pDlgTemplateEx1->style & WS_CHILD)
        out << L"  Child dialog" << std::endl;
    WORD* pMem = Uint16AfterSzOrOrd(pDlgTemplateEx1->menu);
    pMem = Uint16AfterSzOrOrd(pMem);
    out << L"  Caption: " << (const wchar_t*)pMem << std::endl;
    pMem = Uint16AfterSz(pMem); // after title
    pMem += 3; // pointsize, weight, italic, charset
    if (0 != (pDlgTemplateEx1->style & (DS_SETFONT | DS_SHELLFONT)))
    {
        // typeface
        pMem = Uint16AfterSz(pMem);
    }

    for (WORD ixDlgItem = 0; ixDlgItem < nDlgItems; ++ixDlgItem)
    {
        // DLGITEMTEMPLATEEX must be aligned on four-byte boundaries
        while (0 != ((unsigned long long)pMem & 0x03))
            pMem++;

        DLGITEMTEMPLATEEX_1* pDlgItemEx1 = (DLGITEMTEMPLATEEX_1*)pMem;
        out << L"  ID: " << (long)pDlgItemEx1->id << L"  ";
        // Window class
        pMem = pDlgItemEx1->windowClass;
        if (0xFFFF == *pMem)
        {
            switch (pMem[1])
            {
            case 0x0080: out << L"Button"; break;
            case 0x0081: out << L"Edit"; break;
            case 0x0082: out << L"Static"; break;
            case 0x0083: out << L"List box"; break;
            case 0x0084: out << L"Scroll bar"; break;
            case 0x0085: out << L"Combo box"; break;
            default: out << L"Window class ordinal " << pMem[1]; break;
            }
        }
        else
        {
            out << L"Window class " << (const wchar_t*)pDlgItemEx1->windowClass;
        }
        out << L"; title ";
        pMem = Uint16AfterSzOrOrd(pDlgItemEx1->windowClass);
        if (0xFFFF == *pMem)
        {
            out << L"ordinal " << pMem[1] << std::endl;
        }
        else
        {
            out << (const wchar_t*)pMem << std::endl;
        }
        pMem = Uint16AfterSzOrOrd(pMem);
        WORD cbExtra = *pMem++;
        pMem += (cbExtra / 2);
    }
    out << std::endl;

    return true;
}

static bool ValidateDLGTEMPLATE(LPVOID pResource, std::wostream& out, std::wostream& err)
{
    UNREFERENCED_PARAMETER(err);
    DLGTEMPLATE* pDlgTemplate = (DLGTEMPLATE*)pResource;
    WORD nDlgItems = pDlgTemplate->cdit;
    out << L"  " << L"Items: " << nDlgItems << std::endl;
    bool bSetFont = (0 != (pDlgTemplate->style & DS_SETFONT));
    // Menu
    uint16_t* pMem = (uint16_t*)(pDlgTemplate + 1);
    // Window class
    pMem = Uint16AfterSzOrOrd(pMem);
    // Title
    pMem = Uint16AfterSzOrOrd(pMem);
    out << L"  Caption: " << (const wchar_t*)pMem << std::endl;
    pMem = Uint16AfterSz(pMem); // after title
    if (bSetFont)
    {
        pMem = Uint16AfterSz(pMem + 1);
    }

    for (WORD ixDlgItem = 0; ixDlgItem < nDlgItems; ++ixDlgItem)
    {
        // DLGITEMTEMPLATE must be aligned on four-byte boundaries
        while (0 != ((unsigned long long)pMem & 0x03))
            pMem++;

        DLGITEMTEMPLATE* pDlgItem = (DLGITEMTEMPLATE*)pMem;
        out << L"  ID: " << pDlgItem->id << L"  ";
        // window class
        pMem = (uint16_t*)(pDlgItem + 1);

        if (0xFFFF == *pMem)
        {
            switch (pMem[1])
            {
            case 0x0080: out << L"Button"; break;
            case 0x0081: out << L"Edit"; break;
            case 0x0082: out << L"Static"; break;
            case 0x0083: out << L"List box"; break;
            case 0x0084: out << L"Scroll bar"; break;
            case 0x0085: out << L"Combo box"; break;
            default: out << L"Window class ordinal " << pMem[1]; break;
            }
        }
        else
        {
            out << L"Window class " << (const wchar_t*)pMem;
        }
        // title
        out << L"; title ";
        pMem = Uint16AfterSzOrOrd(pMem);
        if (0xFFFF == *pMem)
        {
            out << L"ordinal " << pMem[1] << std::endl;
        }
        else
        {
            out << (const wchar_t*)pMem << std::endl;
        }
        // Creation data
        pMem = Uint16AfterSzOrOrd(pMem);
        WORD cbExtra = *pMem++;
        pMem += (cbExtra / 2);
    }
    out << std::endl;

    return true;
}

#endif