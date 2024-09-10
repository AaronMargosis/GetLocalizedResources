#pragma once

#include "UtilityFunctions.h"

/// <summary>
/// Outputs localized text in the module's menu resources as tab-delimited fields.
/// Output includes the menu ID, control ID, and the localized text both with accelerators
/// and with accelerator characters removed.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool MenuTextExtraction(HMODULE hModule, streams_t& streams);
