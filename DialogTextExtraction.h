#pragma once

#include "UtilityFunctions.h"

/// <summary>
/// Outputs localized text in the module's dialog resources as tab-delimited fields.
/// Output includes the dialog ID, control ID, the localized text both with accelerators
/// and with accelerator characters removed, and the control type.
/// </summary>
/// <param name="hModule">Handle to the module to inspect</param>
/// <param name="streams">The output and error streams to write information into</param>
/// <returns>true if successful, false otherwise.</returns>
bool DialogTextExtraction(HMODULE hModule, streams_t& streams);
