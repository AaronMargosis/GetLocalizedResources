#pragma once

#include <Windows.h>
#include <string>
#include "SysErrorMessage.h"

/// <summary>
/// Class to (temporarily) change the current thread's preferred UI language and 
/// to restore the original language preference.
/// (Implemented entirely inline.)
/// </summary>
class LanguageChanger
{
public:
	LanguageChanger() : 
		m_szzOriginalLanguages(nullptr)
	{
	}

	/// <summary>
	/// On destruction, restore's original language prefs if it had been changed
	/// </summary>
	~LanguageChanger() { Revert(); }

	/// <summary>
	/// Set the thread's preferred UI languages
	/// </summary>
	/// <param name="szLanguages">Language specification(s). E.g., "fr-fr", "ja-jp"</param>
	/// <param name="sErrorInfo">Diagnostic information on failure</param>
	/// <returns>true if successful, false otherwise</returns>
	bool SetLanguage(const wchar_t* szLanguages, std::wstring& sErrorInfo)
	{
		sErrorInfo.clear();

		// Validate input
		if (!szLanguages)
		{
			return false;
		}

		// Save original language prefs if not yet captured
		if (!m_szzOriginalLanguages)
		{
			SaveCurrent();
		}

		return SetLanguage_Internal(szLanguages, sErrorInfo);
	}

	/// <summary>
	/// Restore original language prefs if language had been changed
	/// </summary>
	void Revert()
	{
		if (m_szzOriginalLanguages)
		{
			std::wstring sErrorInfo;
			SetLanguage_Internal(m_szzOriginalLanguages, sErrorInfo);
			Clear();
		}
	}

private:
	/// <summary>
	/// Saves current/original language prefs
	/// </summary>
	void SaveCurrent()
	{
		Clear();

		ULONG ulNumLanguages = 0, cchLanguagesBuffer = 0;
		// Determine how large the buffer needs to be
		GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, &ulNumLanguages, nullptr, &cchLanguagesBuffer);
		if (cchLanguagesBuffer > 0)
		{
			m_szzOriginalLanguages = new wchar_t[cchLanguagesBuffer];
			if (!GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, &ulNumLanguages, m_szzOriginalLanguages, &cchLanguagesBuffer))
			{
				Clear();
			}
		}
	}

	/// <summary>
	/// Sets the thread's preferred UI languages
	/// </summary>
	/// <param name="szLanguages">Preferred UI language(s); e.g., "fr-fr", "ja-jp", "en-us"</param>
	/// <param name="sErrorInfo">Error information on failure</param>
	/// <returns>true if successful, false otherwise</returns>
	bool SetLanguage_Internal(const wchar_t* szLanguages, std::wstring& sErrorInfo)
	{
		// Make sure the lang(s) spec is double-null terminated
		std::wstring sLangs(szLanguages);
		sLangs += L"\0\0";

		BOOL ret = SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, sLangs.c_str(), nullptr);
		if (!ret)
		{
			sErrorInfo = SysErrorMessage();
			return false;
		}
		else
		{
			return true;
		}
	}

	/// <summary>
	/// Release the memory saving the original language prefs
	/// </summary>
	void Clear()
	{
		if (m_szzOriginalLanguages)
		{
			delete[] m_szzOriginalLanguages;
			m_szzOriginalLanguages = nullptr;
		}
	}

private:
	wchar_t* m_szzOriginalLanguages = nullptr;

private:
	// Not implemented
	LanguageChanger(const LanguageChanger&) = delete;
	LanguageChanger& operator = (const LanguageChanger&) = delete;
};

