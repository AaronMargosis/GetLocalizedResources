# GetLocalizedResources
Get localized text from a Portable Executable resource file (e.g., EXE, DLL, .MUI).
Extracts localized text from the named resource file's string table, dialogs,
message table, or menu resources, as tab-delimited text with headers.

Among other things you can find, the message tables of ntdll.dll and kernel32.dll contain
the error message text associated with NTSTATUS and Win32 error codes.

Optionally outputs to a named UTF-8 encoded file, which correctly preserves character
encodings. This option is highly recommended particularly for non-English languages,
as the Windows console subsystem's redirection functionality often does not handle non-ANSI 
encodings well.

Optionally uses another installed language instead of the user's default language.

`GetLocalizedResources.exe` is an x64 executable. `GetLocalizedResources32.exe` is an
x86 executable, but on 64-bit Windows it selectively disables WOW64 file system redirection so it
works correctly when inspecting files in/under the System32 directory.

Command-line syntax:
```
GetLocalizedResources.exe {-s|-d|-m|-n} [-l langspec] [-o outfile] resourceFile

Must pick one of -s, -d, -m, or -n:
  -s   : output contents of string table
  -d   : output text in dialog resources
  -m   : output contents of message table
  -n   : output text in menu resources

  -o   : output to a named UTF-8 file. If -o not used, outputs to stdout.
         (Recommended: much higher fidelity than Windows console redirection
         using ">" or "|", especially with non-English languages.)

  -l langspec
       : use the specified language (if possible) instead of the default language.
         Language specification must be in the "name" form, such as "fr-FR".

  resourceFile
       : the resource PE file (e.g., EXE or DLL) from which to extract resources.
         Full path not required if file is in the path.
         Can be an EXE or DLL, or an associated .mui file.
         If a system file, Windows will get the system's default localized resources.

Examples:
    GetLocalizedResources.exe -d wsecedit.dll -o .\wsecedit-dlg.txt
    GetLocalizedResources.exe -s -o .\wsecedit-strings.txt C:\Windows\System32\fr-FR\wsecedit.dll.mui
    GetLocalizedResources.exe -m msprivs.dll -l fr-FR -o .\msprivs-French.txt
    GetLocalizedResources.exe -m ntdll.dll -o .\AllTheNtstatusErrorMessages.txt
    GetLocalizedResources.exe -m kernel32.dll -o .\LotsOfTheWin32ErrorMessages.txt
```
