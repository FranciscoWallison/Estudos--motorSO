#pragma once
#include <windows.h>
#include <winternl.h>

// THREADINFOCLASS valor 9: ThreadQuerySetWin32StartAddress (não exposto no SDK antigo)
#ifndef ThreadQuerySetWin32StartAddress
#define ThreadQuerySetWin32StartAddress (THREADINFOCLASS)9
#endif

using PFN_NtQueryInformationThread = NTSTATUS (NTAPI*)(
    HANDLE ThreadHandle,
    THREADINFOCLASS ThreadInformationClass,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength,
    PULONG ReturnLength
);

PFN_NtQueryInformationThread resolveNtQueryInformationThread(); // pega de ntdll
bool queryThreadStartAddress(HANDLE hThread, void** outStart);
bool getThreadCurrentAddress(HANDLE hThread, uintptr_t& outAddr);
