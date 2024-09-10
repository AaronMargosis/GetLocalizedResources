#include <Windows.h>
#include <iostream>
#include "StringTableExtraction.h"
#include "UtilityFunctions.h"


/// <summary>
/// Outputs localized text in the module's string table as tab-delimited fields.
/// Output includes the string ID, and the localized text both with accelerators
/// and with accelerator characters removed. CR, LF, TAB, and embedded NUL characters
/// are replaced in the output with \r, \n, \t, and \0.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool StringTableExtraction(HMODULE hModule, streams_t& streams)
{
    // Tab-delimited headers
    streams.WCout
        << L"String ID\t"
        << L"Localized text\t"
        << L"Orig localized text"
        << std::endl;

    // String table IDs must be between 0 and 65535.
    // Because of the way string resources are stored and enumerated (blocks of 16 length-prefixed strings, not
    // zero-terminated, it's far easier just to query for every possible ID and report the ones that we find, 
    // and not terribly time-consuming.
    for (UINT uID = 0; uID < 65536; uID++)
    {
        // Optional use of LoadStringW where the caller doesn't need to preallocate a buffer to fill,
        // passing 0 for the cchBufferMax parameter.
        // The lpBuffer parameter is treated as the address of a wchar_t pointer, and receives a pointer
        // to a read-only buffer containing the string data. That data is not guaranteed to be zero-terminated;
        // the return value indicates how many characters the requested string contains.
        // Note that it is not possible to distinguish between a zero-length string and a non-existent resource;
        // Empty strings aren't interesting and this won't report them.
        wchar_t* pszBuffer = nullptr;
        int ret = LoadStringW(hModule, uID, (LPWSTR) &pszBuffer, 0);
        if (0 != ret && nullptr != pszBuffer)
        {
            // wstring constructor that takes a pointer and the number of characters.
            std::wstring sString(pszBuffer, ret);
            // Replace CR, LF, and TAB with \r, \n, and \t
            sString = escapeCrLfTabNul(sString);
            streams.WCout
                << uID << L"\t"
                << RemoveAccelsFromText(sString) << L"\t"
                << sString
                << std::endl;
        }
    }

    return true;
}
