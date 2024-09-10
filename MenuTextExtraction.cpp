#include <Windows.h>
#include <iostream>
#include "MenuTextExtraction.h"
#include "SysErrorMessage.h"
#include "UtilityFunctions.h"
#include "ResourceDefs.h"
#include "HEX.h"

/*
References:

The evolution of menu templates: Introduction - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20080708-00/?p=21713
The evolution of menu templates: 16-bit classic menus - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20080709-00/?p=21693
The evolution of menu templates: 32-bit classic menus - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20080711-00/?p=21653
The evolution of menu templates: 16-bit extended menus - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20080715-00/?p=21613
The evolution of menu templates: 32-bit extended menus - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20080716-00/?p=21603
What's the deal with that alternate form for menu item template separators? - The Old New Thing (microsoft.com)
https://devblogs.microsoft.com/oldnewthing/20080710-00/?p=21673

*/

/// <summary>
/// Indicates whether the resource is a standard menu template, an extended menu template,
/// or neither.
/// </summary>
/// <param name="pResource">Address of the resource</param>
/// <param name="bIsExtendedMenuTemplate">If successful, true for extended, false for standard. Undefined if not successful.</param>
/// <returns>true if resource determined to be a standard or extended menu template; false if not.</returns>
static bool IsExtendedMenuTemplate(LPVOID pResource, bool& bIsExtendedMenuTemplate)
{
    WORD wVersion = *(WORD*)pResource;
    switch (wVersion)
    {
    case 0:
        bIsExtendedMenuTemplate = false;
        return true;
    case 1:
        bIsExtendedMenuTemplate = true;
        return true;
    default:
        return false;
    }
}

/// <summary>
/// Indicates whether the second memory address is in the range beginning with the first memory address and
/// with the resource size specified.
/// </summary>
/// <param name="pvBaseAddress">Input: base address</param>
/// <param name="dwResourceSize">Input: number of valid bytes following the base address</param>
/// <param name="pMem">Input: address to check</param>
/// <returns>true if pMem is between pvBaseAddress and pvBaseAddress+dwResourceSize; false otherwise</returns>
static inline bool InAddressRange(LPVOID pvBaseAddress, DWORD dwResourceSize, uint16_t* pMem)
{
    byte* pBaseAddress = (byte*)pvBaseAddress;
    byte* pMaxAddress = pBaseAddress + dwResourceSize;
    return ((byte*)pMem >= pBaseAddress && (byte*)pMem < pMaxAddress);
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

/// <summary>
/// Returns the input string terminated at the first tab character.
/// </summary>
static inline std::wstring RemoveTabAndAfter(const std::wstring& sInput)
{
    size_t ix = sInput.find(L'\t', 0);
    if (std::wstring::npos == ix)
        return sInput;
    else
        return sInput.substr(0, ix);
}

/// <summary>
/// Process an extended menu template.
/// Output a line of tab-delimited information for each textual menu item.
/// Helped tremendously by Raymond Chen's "The evolution of menu templates: 16/32-bit extended menus"
/// </summary>
/// <param name="lpName"></param>
/// <param name="pResource"></param>
/// <param name="dwResourceSize"></param>
/// <param name="out"></param>
/// <param name="err"></param>
/// <returns></returns>
static bool ProcessExtendedMenuTemplate(LPCWSTR lpName, LPVOID pResource, DWORD dwResourceSize, std::wostream& out, std::wostream& err)
{
    // Point to the beginning of the menu template
    MENUEX_TEMPLATE_HEADER* pHeader = (MENUEX_TEMPLATE_HEADER*)pResource;

    // This failure NEVER happens
    if (4 != pHeader->wOffset)
        err << L"EXTENDED OFFSET UNEXPECTED VALUE: " << pHeader->wOffset << std::endl;

    // Point to the memory immediately following the header
    uint16_t* pMem = (uint16_t*)(pHeader + 1);

    // Add size of an extra uint16_t before comparing, to make sure the alignment won't push it over
    while (InAddressRange(pResource, dwResourceSize, pMem + 1))
    {
        // Each MENUEX_TEMPLATE_ITEM must be aligned on a four-byte boundary
        while (0 != ((unsigned long long)pMem & 0x03))
            pMem++;

        // Point to the extended menu template item
        MENUEX_TEMPLATE_ITEM* pMenuItem = (MENUEX_TEMPLATE_ITEM*)pMem;
        // There is no szText member if the menu item is a separator or a bitmap
        bool bNoText = 0 != (pMenuItem->dwType & (MFT_SEPARATOR | MFT_BITMAP));
        // Popup is followed by a four-byte header structure preceding the popup menu items
        bool bPopup = 0 != (pMenuItem->wFlags & 0x01);
        // Look for text only if it can be there
        if (!bNoText)
        {
            // If there's non-empty text, output a line of tab-delimited info
            std::wstring sText = pMenuItem->szText;
            if (sText.length() > 0)
            {
                // Name/ID of menu
                // Control ID for the menu item
                // Localized text, with ampersand accelerators removed
                // Original text, with ampersands not removed
                out
                    << RSRCID_t(lpName) << L"\t"
                    << (INT)pMenuItem->uId << L"\t"
                    << RemoveAccelsFromText(sText) << L"\t"
                    << sText
                    << std::endl;
            }
        }

        // Point to the next extended menu item
        if (bNoText)
            // No szText member, so point to where it would have been
            pMem = (uint16_t*)pMenuItem->szText;
        else if (bPopup)
            // After the text, and a four-byte (two uint16_t) header
            pMem = Uint16AfterSz((uint16_t*)(pMenuItem->szText)) + 2;
        else
            // After the text
            pMem = Uint16AfterSz((uint16_t*)(pMenuItem->szText));
    }

    return true;
}

//void FlagsToStream(std::wostream& os, WORD wFlags)
//{
//    os << HEX(wFlags, 4, false, true);
//    if (wFlags & MF_POPUP)
//        os << L"[popup]";
//    if (wFlags & MF_END)
//        os << L"[end]";
//    if (wFlags & MF_GRAYED)
//        os << L"[grayed]";
//    if (wFlags & MF_DISABLED)
//        os << L"[disabled]";
//}

/// <summary>
/// Process a standard/"classic" menu template.
/// Output a line of tab-delimited information for each textual menu item.
/// Helped tremendously by Raymond Chen's "The evolution of menu templates: 16/32-bit classic menus"
/// </summary>
/// <param name="lpName"></param>
/// <param name="pResource"></param>
/// <param name="dwResourceSize"></param>
/// <param name="out"></param>
/// <param name="err"></param>
/// <returns></returns>
static bool ProcessStandardMenuTemplate(LPCWSTR lpName, LPVOID pResource, DWORD dwResourceSize, std::wostream& out, std::wostream& err)
{
    // Point to the beginning of the menu template
    MENUHEADER* pHeader = (MENUHEADER*)pResource;

    // This failure NEVER happens
    if (0 != pHeader->cbHeaderSize)
        err << L"STANDARD CBHEADERSIZE UNEXPECTED VALUE: " << pHeader->cbHeaderSize << std::endl;

    // Point to the memory immediately following the header
    uint16_t* pMem = (uint16_t*)(pHeader + 1);

    // Add size of an extra uint16_t before comparing
    while (InAddressRange(pResource, dwResourceSize, pMem + 1))
    {
        // First word is flags, which indicates whether it's a popup or an item with a control ID
        WORD wFlags = *pMem++;
        if (wFlags & MF_POPUP)
        {
            // It's a popup. No control ID. Menu text starts right after the flags.
            std::wstring sText = (const wchar_t*)pMem;

            // If non-empty, write out a line of tab-delimited information
            if (sText.length() > 0)
            {
                // Tab character is used to add an accelerator key combo to the menu entry.
                // Almost certainly don't need to worry about those in popups, but check anyway.
                sText = RemoveTabAndAfter(sText);

                // Name/ID of menu
                // No control ID for popup, so write "n/a"
                // Localized text, with ampersand accelerators removed
                // Original text, with ampersands not removed
                out
                    << RSRCID_t(lpName) << L"\t"
                    << L"n/a" << L"\t"
                    << RemoveAccelsFromText(sText) << L"\t"
                    << sText
                    << std::endl;
            }
        }
        else
        {
            // Not a popup; next word is the menu item's control ID, followed by the menu text.
            WORD wID = *pMem++;
            std::wstring sText = (const wchar_t*)pMem;

            // If non-empty, write out a line of tab-delimited information
            if (sText.length() > 0)
            {
                // Tab character is used to add an accelerator key combo to the menu entry.
                sText = RemoveTabAndAfter(sText);

                // Name/ID of menu
                // Control ID for the menu item
                // Localized text, with ampersand accelerators removed
                // Original text, with ampersands not removed
                out
                    << RSRCID_t(lpName) << L"\t"
                    << wID << L"\t"
                    << RemoveAccelsFromText(sText) << L"\t"
                    << sText
                    << std::endl;
            }
        }
        // Point to the next menu item, which follows the text that pMem is pointing to.
        pMem = Uint16AfterSz(pMem);
    }

    return true;
}

/// <summary>
/// Callback function to handle each menu resource in the current file.
/// </summary>
/// <param name="hModule">Handle to the resource DLL</param>
/// <param name="lpType">Resource type</param>
/// <param name="lpName">Resource name/identifier</param>
/// <param name="lParam">Pointer to output stream</param>
/// <returns>Always returns TRUE to continue enumeration</returns>
static BOOL CALLBACK EnumMenuCallbackProc(
    _In_opt_ HMODULE hModule,
    _In_ LPCWSTR lpType,
    _In_ LPWSTR lpName,
    _In_ LONG_PTR lParam)
{
    streams_t* pStreams = (streams_t*)lParam;
    // Should enumerate only RT_MENUs, but check again just to be safe
    if (RT_MENU == lpType)
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
                bool bValid, bIsExtendedMenuTemplate;
                bValid = IsExtendedMenuTemplate(pData, bIsExtendedMenuTemplate);
                if (!bValid)
                {
                    pStreams->WCerr << L"INVALID MENU, WTAF" << std::endl;
                }
                else
                {
                    if (bIsExtendedMenuTemplate)
                    {
                        ProcessExtendedMenuTemplate(lpName, pData, dwResourceSize, pStreams->WCout, pStreams->WCerr);
                    }
                    else
                    {
                        ProcessStandardMenuTemplate(lpName, pData, dwResourceSize, pStreams->WCout, pStreams->WCerr);
                    }
                }
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
/// Outputs localized text in the module's menu resources as tab-delimited fields.
/// Output includes the menu ID, control ID, and the localized text both with accelerators
/// and with accelerator characters removed.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool MenuTextExtraction(HMODULE hModule, streams_t& streams)
{
    // Tab-delimited headers
    streams.WCout
        << L"Menu ID\t"
        << L"Ctrl ID\t"
        << L"Localized text\t"
        << L"Dialog text"
        << std::endl;

    // Enumerate the menu resources
    if (!EnumResourceNamesW(hModule, RT_MENU, EnumMenuCallbackProc, (LPARAM)&streams))
    {
        DWORD dwLastErr = GetLastError();
        streams.WCerr << L"EnumResourceNamesW failed: " << SysErrorMessageWithCode(dwLastErr) << std::endl;
        return false;
    }

    return true;
}
