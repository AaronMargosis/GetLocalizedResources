#pragma once

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
bool StringTableExtraction(HMODULE hModule, streams_t& streams);
