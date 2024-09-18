// GetLocalizedResources.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include "FileOutput.h"
#include "DialogTextExtraction.h"
#include "StringTableExtraction.h"
#include "MessageTableExtraction.h"
#include "MenuTextExtraction.h"
#include "SysErrorMessage.h"
#include "UtilityFunctions.h"
#include "LanguageChanger.h"
#include "Wow64FsRedirection.h"

/// <summary>
/// Write command-line syntax to stderr and then exit.
/// </summary>
/// <param name="argv0">Input: The program's argv[0] value</param>
/// <param name="szError">Optional input: caller-supplied error text</param>
static void Usage(const wchar_t* argv0, const wchar_t* szError = nullptr)
{
	std::wstring sExe = GetFileNameFromFilePath(argv0);
	if (szError)
		std::wcerr << szError << std::endl;
	std::wcerr
		<< std::endl
		<< sExe << L":" << std::endl
		<< L"    Extracts localized text from the named file's string table, dialogs, message table, or menu resources," << std::endl
		<< L"    as tab-delimited text with headers." << std::endl
		<< std::endl
		<< L"Usage:" << std::endl
		<< std::endl
		<< L"    " << sExe << L" {-s|-d|-m|-n} [-l langspec] [-o outfile] resourceFile" << std::endl
		<< std::endl
		<< L"Must pick one of -s, -d, -m, or -n:" << std::endl
		<< L"  -s   : output contents of string table" << std::endl
		<< L"  -d   : output text in dialog resources" << std::endl
		<< L"  -m   : output contents of message table" << std::endl
		<< L"  -n   : output text in menu resources" << std::endl
		<< std::endl
		<< L"  -o   : output to a named UTF-8 file. If -o not used, outputs to stdout." << std::endl
		<< L"         (Recommended: much higher fidelity than Windows console redirection" << std::endl
		<< L"         using \">\" or \"|\", especially with non-English languages.)" << std::endl
		<< std::endl
		<< L"  -l langspec" << std::endl
		<< L"       : use the specified language (if possible) instead of the default language." << std::endl
		<< L"         Language specification must be in the \"name\" form, such as \"fr-FR\"." << std::endl
		<< std::endl
		<< L"  resourceFile" << std::endl
		<< L"       : the resource PE file (e.g., EXE or DLL) from which to extract resources." << std::endl
		<< L"         Full path not required if file is in the path." << std::endl
		<< L"         Can be an EXE or DLL, or an associated .mui file." << std::endl
		<< L"         If a system file, Windows will get the system's default localized resources." << std::endl
		<< std::endl
		<< L"Examples:" << std::endl
		<< L"    " << sExe << L" -d wsecedit.dll -o .\\wsecedit-dlg.txt" << std::endl
		<< L"    " << sExe << L" -s -o .\\wsecedit-strings.txt C:\\Windows\\System32\\fr-FR\\wsecedit.dll.mui" << std::endl
		<< L"    " << sExe << L" -m msprivs.dll -l fr-FR -o .\\msprivs-French.txt" << std::endl
		<< L"    " << sExe << L" -m ntdll.dll -o .\\AllTheNtstatusErrorMessages.txt" << std::endl
		<< L"    " << sExe << L" -m kernel32.dll -o .\\LotsOfTheWin32ErrorMessages.txt" << std::endl
		<< std::endl;
	exit(-1);
}

int wmain(int argc, wchar_t** argv)
{
	// Set output mode to UTF8.
	if (_setmode(_fileno(stdout), _O_U8TEXT) == -1 || _setmode(_fileno(stderr), _O_U8TEXT) == -1)
	{
		std::wcerr << L"Unable to set stdout and/or stderr modes to UTF8." << std::endl;
	}

	bool bOut_toFile = false;
	std::wstring sOutFile, sResourceFile, sLangSpec;
	enum class option_t
	{
		eNotSet,
		eStringTable,
		eDialog,
		eMessageTable,
		eMenu
	} option = option_t::eNotSet;

	// Optional redirection for stdout and stderr.
	// Note that particularly for non-USEnglish languages, output SHOULD be sent directly to a UTF-8 or Unicode file 
	// rather than to stdout, as the console does not always correctly handle that output (esp. in PowerShell console).
	// Default to stdout, stderr
	std::wostream* pWCout = &std::wcout;
	std::wostream* pWCerr = &std::wcerr;
	// Option for file output streams.
	std::wofstream fOut, fErr;
	bool bCloseFOut = false, bCloseFErr = false;

	LanguageChanger languageChanger;

	// Process command-line arguments
	int ixArg = 1;
	while (ixArg < argc)
	{
		//TODO: make sure that user doesn't try to set more than one of the options -s -d -m -n
		if (0 == wcscmp(L"-s", argv[ixArg]))
			option = option_t::eStringTable;
		else if (0 == wcscmp(L"-d", argv[ixArg]))
			option = option_t::eDialog;
		else if (0 == wcscmp(L"-m", argv[ixArg]))
			option = option_t::eMessageTable;
		else if (0 == wcscmp(L"-n", argv[ixArg]))
			option = option_t::eMenu;
		else if (0 == wcscmp(L"-o", argv[ixArg]))
		{
			// Check for already set output file
			if (bOut_toFile)
				Usage(argv[0], L"Output file specified multiple times");
			bOut_toFile = true;
			if (++ixArg >= argc)
				Usage(argv[0], L"Missing arg for -o");
			sOutFile = argv[ixArg];
		}
		else if (0 == wcscmp(L"-l", argv[ixArg]))
		{
			// Check for lang already specified
			if (sLangSpec.length() > 0)
				Usage(argv[0], L"Language specified multiple times");
			if (++ixArg >= argc)
				Usage(argv[0], L"Missing arg for -l");
			sLangSpec = argv[ixArg];
		}
		else
		{
			// Already set file name
			if (sResourceFile.length() > 0)
				Usage(argv[0]);
			sResourceFile = argv[ixArg];
		}
		++ixArg;
	}
	// Validate command line
	if (option_t::eNotSet == option)
		Usage(argv[0], L"Option not specified.");
	if (0 == sResourceFile.length())
		Usage(argv[0], L"Resource file not specified.");

	// If language specified, switch to it
	if (sLangSpec.length() > 0)
	{
		std::wstring sErrorInfo;
		if (!languageChanger.SetLanguage(sLangSpec.c_str(), sErrorInfo))
		{
			std::wstring sErrText = L"Language not set: " + sErrorInfo;
			Usage(argv[0], sErrText.c_str());
		}
	}

	// Load the resource file. 
	// Temporarily disable WOW64 file system redirection so if this is a 32-bit process it can still
	// access resources in the System32 directory on 64-bit Windows.
	Wow64FsRedirection fsRedir;
	fsRedir.Disable();
	HMODULE hModule = LoadLibraryExW(sResourceFile.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
	DWORD dwLastErr = GetLastError();
	fsRedir.Revert();
	if (!hModule)
	{
		std::wcerr 
			<< L"Cannot load resource file " << sResourceFile << std::endl
			<< SysErrorMessageWithCode(dwLastErr) << std::endl;
		Usage(argv[0]);
	}

	// Set up output file if specified.
	if (bOut_toFile)
	{
		// Maybe not a good idea to write the output in/under the System32 directory, but allow it
		// rather than redirect to SysWOW64.
		fsRedir.Disable();
		bool bFileCreated = CreateFileOutput(sOutFile.c_str(), fOut);
		fsRedir.Revert();
		if (bFileCreated)
		{
			pWCout = &fOut;
			bCloseFOut = true;
		}
		else
		{
			std::wcerr << L"Error: Couldn't open output file " << sOutFile << std::endl;
			FreeLibrary(hModule);
			Usage(argv[0]);
		}
	}

	// Single object specifying output and error streams
	streams_t streams(*pWCout, *pWCerr);

	// Do the work...
	switch (option)
	{
	case option_t::eStringTable:
		StringTableExtraction(hModule, streams);
		break;
	case option_t::eDialog:
		DialogTextExtraction(hModule, streams);
		break;
	case option_t::eMessageTable:
		MessageTableExtraction(hModule, streams);
		break;
	case option_t::eMenu:
		MenuTextExtraction(hModule, streams);
		break;
	default:
		streams.WCerr << L"This option doesn't exist - WTAF? " << (int)option << std::endl;
		break;
	}

// Eliminate false positive about hModule possibly being 0
#pragma warning(push)
#pragma warning (disable: 6387)
	FreeLibrary(hModule);
#pragma warning(pop)

	if (bCloseFOut)
		fOut.close();
	if (bCloseFErr)
		fErr.close();

	return 0;
}

