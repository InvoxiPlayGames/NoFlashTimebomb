#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>

typedef enum _PatchFlashRcode
{
	resSuccess = 1,
	resNotNeeded = 0,
	resFailed = -1,
	resInvalidParameter = -2
} PatchFlashRcode;

#ifdef _M_AMD64 // x86_64
/*
  call get_time_func -> nop
  comisd xmm0, qword ptr [rip + offset timebombPtr] -> nop
  mov rcx, rbx ==
  setnc al -> nop + mov al, 0
*/
BYTE timebombSearchByte[] = { 0xE8, 0x00, 0x00, 0x00, 0x00, 0x66, 0x0f, 0x2f, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xcb, 0x0f, 0x93, 0xc0 };
BYTE timebombSearchMask[] = { 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

BYTE timebombReplacByte[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0x00, 0x90, 0xb0, 0x00 };
BYTE timebombReplacMask[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff };
#else // i386
/*
  fcomp qword ptr [timebombPtr] -> nop
  fnstsw ax -> nop
  pop ecx ==
  pop ecx ==
  test ah, 0x1 -> nop
  jnz a_mov_eax_ebx -> jmp a_mov_eax_ebx
  xor eax, eax -> nop
  inc eax -> nop
  jmp a_mov_al_to_esi_struct -> nop
*/
BYTE timebombSearchByte[] = { 0xdc, 0x1d, 0x00, 0x00, 0x00, 0x00, 0xdf, 0xe0, 0x59, 0x59, 0xf6, 0xc4, 0x01, 0x75, 0x00, 0x33, 0xc0, 0x40, 0xeb, 0x00 };
BYTE timebombSearchMask[] = { 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00 };

BYTE timebombReplacByte[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0x90, 0x90, 0x90, 0xeb, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90 };
BYTE timebombReplacMask[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff };
#endif

__declspec(dllexport) int PatchFlashTimebombHandle(HMODULE hModule)
{
	MODULEINFO mModuleInfo;
	PVOID lpBaseAddress = NULL;
	DWORD dwImageSize = 0;
	PVOID lpFoundAddress = NULL;

	// make sure we've got an actual module
	if (hModule == NULL)
	{
		return resInvalidParameter;
	}

	// get the base address and size of the flash player module
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &mModuleInfo, sizeof(MODULEINFO)))
	{
		return resFailed;
	}
	lpBaseAddress = mModuleInfo.lpBaseOfDll;
	dwImageSize = mModuleInfo.SizeOfImage;

	// start the search
	{
		DWORD i = 0;
		DWORD dwProgress = 0;
		PBYTE lpSearchBytes = (PBYTE)lpBaseAddress;
		// this is slow - enumerate through every byte and see if we've got a match for our mask
		for (i = 0; i < dwImageSize - sizeof(timebombSearchByte); i++)
		{
			if ((lpSearchBytes[i] & timebombSearchMask[dwProgress]) == timebombSearchByte[dwProgress])
			{
				dwProgress++;

				if (dwProgress == sizeof(timebombSearchByte))
				{
					lpFoundAddress = (PVOID)(lpSearchBytes + i - sizeof(timebombSearchByte) + 1);
					break;
				}
			}
			else
			{
				dwProgress = 0;
			}
		}
	}

	// if the search was successful, patch
	if (lpFoundAddress != NULL)
	{
		DWORD dwOldProtect = 0;
		DWORD dwDiscard = 0;
		DWORD i = 0;
		PBYTE lpPatchBytes = (PBYTE)lpFoundAddress;

		// set the page to RWX so we can patch it
		VirtualProtect(lpPatchBytes, sizeof(timebombReplacByte), PAGE_EXECUTE_READWRITE, &dwOldProtect);

		for (i = 0; i < sizeof(timebombReplacByte); i++)
		{
			// technically not a mask
			if (timebombReplacMask[i] == 0xFF)
			{
				lpPatchBytes[i] = timebombReplacByte[i];
			}
		}

		// restore the permissions so our flash isn't in a 
		VirtualProtect(lpPatchBytes, sizeof(timebombReplacByte), dwOldProtect, &dwDiscard);

		return resSuccess;
	}

	return resNotNeeded;
}

__declspec(dllexport) int PatchFlashTimebombA(LPCSTR lpModuleName)
{
	HMODULE hModule = NULL;

	if (lpModuleName == NULL)
	{
		return resInvalidParameter;
	}

	hModule = GetModuleHandleA(lpModuleName);

	return PatchFlashTimebombHandle(hModule);
}

__declspec(dllexport) int PatchFlashTimebombW(LPCWSTR lpModuleName)
{
	HMODULE hModule = NULL;

	if (lpModuleName == NULL)
	{
		return resInvalidParameter;
	}

	hModule = GetModuleHandleW(lpModuleName);

	return PatchFlashTimebombHandle(hModule);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
