# GetLocalizedResources.exe
Get localized text from a Portable Executable resource file (e.g., EXE or DLL).
Extracts localized text from the named resource file's string table, dialogs,
message table, or menu resources, as tab-delimited text with headers.

Optionally outputs to a named UTF-8 encoded file, which correctly preserves character
encodings. This option is highly recommended particularly for non-English languages,
as the Windows console subsystem's redirection functionality does not handle non-ANSI 
encodings well.

Optionally uses another installed language instead of the user's default language.

x64 build only.

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
```
