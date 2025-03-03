# NoFlashTimebomb

Really small DLL to patch Flash Player's OCX/DLL at runtime to remove the
timebomb, rather than patching the file on-disk. This is for developers of
embedded Flash applications to use, not for end users unfortunately.

Flash has been deprecated for over half a decade, I made this just for a fun
challenge for [NGPlayerNET](https://github.com/InvoxiPlayGames/NGPlayerNET),
and because I think pre-patched modules are antithetical to true "preservation" - 
it's destructive and it breaks any confirmation of the original source (no
digital signature!)

I haven't tested this on anything other than the ActiveX modules for Flash 32
r465, both 32-bit and 64-bit.

## Building

Visual Studio 2010 (or later) with the Visual C++ build tools installed.

## Developer Usage

This DLL exports three functions:

```c
typedef enum _PatchFlashRcode
{
	resSuccess = 1,
	resNotNeeded = 0,
	resFailed = -1,
	resInvalidParameter = -2
} PatchFlashRcode;

extern int PatchFlashTimebombHandle(HMODULE hModule);

extern int PatchFlashTimebombA(LPCSTR lpModuleName);

extern int PatchFlashTimebombW(LPCWSTR lpModuleName);
```

You can either pass in the HMODULE of the Flash plugin, or the module name of
the Flash module.

## License

MIT - see LICENSE.txt.
