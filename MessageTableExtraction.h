#pragma once

#include "UtilityFunctions.h"

/// <summary>
/// Outputs localized text in the module's message table resource as tab-delimited fields.
/// Output includes the message ID in decimal and hex, and the localized text.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool MessageTableExtraction(HMODULE hModule, streams_t& streams);
