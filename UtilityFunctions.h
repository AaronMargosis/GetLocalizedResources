#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <regex>
#include "StringUtils.h"


/// <summary>
/// Remove accelerator characters (&) from text, while leaving escaped ampersands in place.
/// Also remove East Asian-language accelerator patterns.
/// </summary>
/// <param name="sInput">Input: text from dialog resource, possibly with accelerator characters</param>
/// <returns>Input string with unescaped accelerator characters removed.</returns>
inline std::wstring RemoveAccelsFromText(const std::wstring& sInput)
{
    // From what I have observed, strings that are localized in languages that use an Input Method Editor (IME) such
    // as Japanese and Korean and that specify an accelerator using a Latin character do so by showing the Latin
    // character underlined and within parentheses. As with English and most other languages, the underline is 
    // achieved by placing an ampersand before the Latin character in the localized string. For example:
    //   削除(&R)
    // Removing these accelerators from the localized string requires removing the parentheses and the Latin
    // character in addition to the ampersand.

    // Pattern to search for: Left parenthesis character, ampersand character, capital letter A-Z or digit 0-9, right parenthesis character.
    std::wregex regexAsianAccelerator(L"\\(&[A-Z0-9]\\)");
    // Remove anything that matches the Asian accelerator pattern:
    std::wstring sEastAsianAccelsRemoved = std::regex_replace(sInput, regexAsianAccelerator, L"");

    // Now, remove any remaining ampersands unless they are escaped (two consecutive ampersands).
    // Replace escaped ampersands with an unusual string combo, then remove remaining ampersands, and
    // finally restore the escaped ampersands.
    const wchar_t* szEscapedAmpersand = L"&&";
    const std::wstring sTempReplacement = L"~`~";
    return
        replaceStringAll(
            replaceStringAll(
                replaceStringAll(sEastAsianAccelsRemoved, szEscapedAmpersand, sTempReplacement),
                L"&", L""),
            sTempReplacement, szEscapedAmpersand);
}

// --------------------------------------------------------------------------------------------------------------

/// <summary>
/// Resource IDs can be a name or an integer ID. 
/// RSRCID_t is a wrapper to simplify passing it to an ostream without having to check IS_INTRESOURCE every time.
/// </summary>
struct RSRCID_t
{
    RSRCID_t(LPCWSTR lpName) : m_lpName(lpName) {}
    LPCWSTR m_lpName;
};

/// <summary>
/// Output a resource identifier, which can be a name or an integer ID.
/// </summary>
inline std::wostream& operator << (std::wostream& os, const RSRCID_t& d)
{
    if (IS_INTRESOURCE(d.m_lpName))
    {
        os << (unsigned long long)d.m_lpName;
    }
    else
    {
        os << d.m_lpName;
    }
    return os;
}

// --------------------------------------------------------------------------------------------------------------
/// <summary>
/// Structure that contains pointers to wostreams for output and error.
/// Enables passing the two items using a single address.
/// </summary>
struct streams_t
{
    /// <summary>
    /// Structure that contains pointers to wostreams for output and error.
    /// Enables passing the two items using a single address.
    /// </summary>
    streams_t(std::wostream& rOut, std::wostream& rErr) : WCout(rOut), WCerr(rErr) {}

    std::wostream& WCout;
    std::wostream& WCerr;
};
