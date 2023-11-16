#include "stdafx.h"

#include <GWCA/Utilities/Scanner.h>

#include <Base/HelperBox.h>
#include <Logger.h>

DWORD WINAPI init(HMODULE hModule) noexcept
{
    if (!Log::InitializeLog())
    {
        MessageBoxA(
            0,
            "Failed to create outgoing log file.\nThis could be due to a file permissions error or antivirus blocking.",
            "Clientside Error Detected",
            0);
        FreeLibraryAndExitThread(hModule, EXIT_SUCCESS);
    }

    Log::Log("Waiting for logged character\n");

    GW::Scanner::Initialize();

    auto found = (DWORD **)GW::Scanner::Find("\xA3\x00\x00\x00\x00\xFF\x75\x0C\xC7\x05", "x????xxxxx", +1);
    if (!(found && *found))
    {
        MessageBoxA(0,
                    "We can't determine if the character is ingame.\nContact the developers.",
                    "Clientside Error Detected",
                    0);
        FreeLibraryAndExitThread(hModule, EXIT_SUCCESS);
    }

    printf("[SCAN] is_ingame = %p\n", found);

    auto is_ingame = *found;
    while (*is_ingame == 0)
    {
        Sleep(100);
    }

    Log::Log("Creating HelperBox thread\n");

    SafeThreadEntry(hModule);

    return 0;
}

BOOL WINAPI DllMain(_In_ HMODULE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID)
{
    DisableThreadLibraryCalls(_HDllHandle);
    if (_Reason == DLL_PROCESS_ATTACH)
    {
        auto hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)init, _HDllHandle, 0, 0);

        if (hThread != NULL)
            CloseHandle(hThread);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) const char *HelperBoxVersion = HELPERBOXDLL_VERSION;
