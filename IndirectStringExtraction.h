#pragma once

#include "UtilityFunctions.h"

/// <summary>
/// Calls the Windows API that translates an indirect-string reference into human-language text
/// and outputs it to (possibly redirected) stdout.
/// </summary>
/// <param name="sResource">The indirect string reference</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool IndirectStringExtraction(const std::wstring& sResource, streams_t& streams);
