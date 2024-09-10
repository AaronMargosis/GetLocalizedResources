#include <Windows.h>
#include <codecvt>
#include <iostream>
#include "SysErrorMessage.h"
#include "UtilityFunctions.h"
#include "ResourceDefs.h"
#include "HEX.h"
#include "MessageTableExtraction.h"

/// <summary>
/// Indicates whether the second memory address is in the range beginning with the first memory address and
/// with the resource size specified.
/// </summary>
/// <param name="pvBaseAddress">Input: base address</param>
/// <param name="dwResourceSize">Input: number of valid bytes following the base address</param>
/// <param name="pMem">Input: address to check</param>
/// <returns>true if pMem is between pvBaseAddress and pvBaseAddress+dwResourceSize; false otherwise</returns>
static inline bool InAddressRange(const void* pvBaseAddress, DWORD dwResourceSize, const void* pMem)
{
    return (pMem >= pvBaseAddress && pMem < (byte*)pvBaseAddress + dwResourceSize);
}


/// <summary>
/// Callback function to handle each message table resource in the current file.
/// </summary>
/// <param name="hModule">Handle to the resource DLL</param>
/// <param name="lpType">Resource type</param>
/// <param name="lpName">Resource name/identifier</param>
/// <param name="lParam">Pointer to output stream</param>
/// <returns>Always returns TRUE to continue enumeration</returns>
static BOOL CALLBACK EnumMessageTableCallbackProc(
    _In_opt_ HMODULE hModule,
    _In_ LPCWSTR lpType,
    _In_ LPWSTR lpName,
    _In_ LONG_PTR lParam)
{
    streams_t* pStreams = (streams_t*)lParam;
    // Should enumerate only RT_MESSAGETABLE, but check again just to be safe
    if (RT_MESSAGETABLE == lpType)
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
                LPVOID pvData = LockResource(hGbl);
                MESSAGE_RESOURCE_DATA* pData = (MESSAGE_RESOURCE_DATA*)pvData;
                for (DWORD ixBlock = 0; ixBlock < pData->NumberOfBlocks; ++ixBlock)
                {
                    MESSAGE_RESOURCE_BLOCK& block = pData->Blocks[ixBlock];
                    MESSAGE_RESOURCE_ENTRY* pEntry = (MESSAGE_RESOURCE_ENTRY*)((byte*)pData + block.OffsetToEntries);
                    for (DWORD ixEntry = block.LowId; ixEntry <= block.HighId; ++ixEntry)
                    {
                        if (!InAddressRange(pvData, dwResourceSize, pEntry))
                        {
                            pStreams->WCerr << L"Error: address out of range" << std::endl;
                            return FALSE;
                        }

                        pStreams->WCout 
                            << ixEntry << L"\t" 
                            << HEX(ixEntry, 8, true, true) << L"\t";
                        if (pEntry->Flags & MESSAGE_RESOURCE_UNICODE)
                        {
                            // Message text is not guaranteed to be zero-terminated, but it might be.
                            // Don't include any trailing null characters in the output string.
                            const wchar_t* szText = (const wchar_t*)pEntry->Text;
                            // Initial string length. pEntry->Length is length of the entire structure, including two WORD values (Length and Flags).
                            // Subtract those out.
                            size_t nChars = (pEntry->Length - 2 * (sizeof(WORD))) / sizeof(wchar_t);
                            // Decrement while the last character is a null char
                            while (nChars > 0 && 0 == szText[nChars - 1])
                                nChars--;
                            // Create a string with the specified number of characters.
                            std::wstring str(szText, nChars);
                            str = escapeCrLfTab(str);
                            pStreams->WCout << str << std::endl;
                        }
                        else if (pEntry->Flags & MESSAGE_RESOURCE_UTF8)
                        {
                            pStreams->WCout << L"[[[UTF-8 text (not supported)]]]" << std::endl;
                        }
                        else if (0 == pEntry->Flags)
                        {
                            // ANSI text.
                            // Message text is not guaranteed to be zero-terminated, but it might be.
                            // Don't include any trailing null characters in the output string.
                            const char* szText = (const char*)pEntry->Text;
                            // Initial string length. pEntry->Length is length of the entire structure, including two WORD values (Length and Flags).
                            // Subtract those out.
                            size_t nChars = pEntry->Length - (2 * sizeof(WORD));
                            // Decrement while the last character is a null char
                            while (nChars > 0 && 0 == szText[nChars - 1])
                                nChars--;
                            // Create a string with the specified number of characters.
                            std::string str(szText, nChars);
                            // Replace CR, LF, and tab with escaped representations
                            str = escapeCrLfTab(str);
                            // Convert to wstring and output
                            pStreams->WCout << std::wstring_convert< std::codecvt_utf8_utf16< wchar_t > >().from_bytes(str) << std::endl;
                        }
                        else
                        {
                            pStreams->WCout << L"[[[Unexpected flags value " << HEX(pEntry->Flags, 4, false, true) << L"]]]" << std::endl;
                        }

                        pEntry = (MESSAGE_RESOURCE_ENTRY*)((byte*)pEntry + pEntry->Length);
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
/// Outputs localized text in the module's message table resource as tab-delimited fields.
/// Output includes the message ID in decimal and hex, and the localized text.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool MessageTableExtraction(HMODULE hModule, streams_t& streams)
{
    // Tab-delimited headers
    streams.WCout
        << L"Msg ID\t"
        << L"Msg ID (hex)\t"
        << L"Localized text"
        << std::endl;

    // Enumerate the messagetable resources
    if (!EnumResourceNamesW(hModule, RT_MESSAGETABLE, EnumMessageTableCallbackProc, (LPARAM)&streams))
    {
        DWORD dwLastErr = GetLastError();
        streams.WCerr << L"EnumResourceNamesW failed: " << SysErrorMessageWithCode(dwLastErr) << std::endl;
        return false;
    }
    return true;
}
