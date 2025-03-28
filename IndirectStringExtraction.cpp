#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <vector>
#include "SysErrorMessage.h"
#include "IndirectStringExtraction.h"

/// <summary>
/// Calls the Windows API that translates an indirect-string reference into human-language text
/// and outputs it to (possibly redirected) stdout.
/// </summary>
/// <param name="sResource">The indirect string reference</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool IndirectStringExtraction(const std::wstring& sResource, streams_t& streams)
{
	const size_t bufSize = 16384;
	std::vector<wchar_t> vBuffer(bufSize);
	HRESULT hr = SHLoadIndirectString(sResource.c_str(), &vBuffer[0], bufSize, nullptr);
	if (S_OK == hr)
	{
		streams.WCout << (const wchar_t*)&vBuffer[0] << std::endl;
		return true;
	}
	else
	{
		streams.WCerr << SysErrorMessageWithCode(hr) << std::endl;
		return false;
	}
}
