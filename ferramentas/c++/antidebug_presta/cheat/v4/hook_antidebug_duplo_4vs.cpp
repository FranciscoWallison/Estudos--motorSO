#include <windows.h>
#include "MinHook.h"
#include "logger.h"

// --- 1. Typedefs e Ponteiros Originais ---

// Funções da KERNEL32.DLL
using pIsDebuggerPresent = BOOL(WINAPI*)();
using pCheckRemoteDebuggerPresent = BOOL(WINAPI*)(HANDLE, PBOOL);
using pOpenProcess = HANDLE(WINAPI*)(DWORD, BOOL, DWORD);
using pReadProcessMemory = BOOL(WINAPI*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
using pSetUnhandledExceptionFilter = LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI*)(LPTOP_LEVEL_EXCEPTION_FILTER);
using pVirtualProtect = BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD);
using pVirtualAllocEx = LPVOID(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
using pVirtualProtectEx = BOOL(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, PDWORD);
using pWriteProcessMemory = BOOL(WINAPI*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);

// advapi32.dll
using pOpenProcessToken = BOOL(WINAPI*)(HANDLE, DWORD, PHANDLE);
using pAdjustTokenPrivileges = BOOL(WINAPI*)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);
using pImpersonateLoggedOnUser = BOOL(WINAPI*)(HANDLE);
using pCreateProcessWithTokenW = BOOL(WINAPI*)(HANDLE, DWORD, LPCWSTR, LPWSTR, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);

// ntdll.dll
#ifndef NTSTATUS
using NTSTATUS = LONG;
#endif
using pNtReadVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
using pNtWriteVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
using pNtAllocateVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);


enum PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessDebugPort = 7,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessExecuteFlags = 34
};

using pNtQueryInformationProcess = NTSTATUS (NTAPI*)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

// Ponteiros para funções originais
// Funções da KERNEL32.DLL
static pIsDebuggerPresent originalIsDebuggerPresent = nullptr;
static pCheckRemoteDebuggerPresent originalCheckRemoteDebuggerPresent = nullptr;
static pOpenProcess originalOpenProcess = nullptr;
static pReadProcessMemory originalReadProcessMemory = nullptr;
static pSetUnhandledExceptionFilter originalSetUnhandledExceptionFilter = nullptr;
static pVirtualProtect originalVirtualProtect = nullptr;
static pNtQueryInformationProcess originalNtQueryInformationProcess = nullptr;
static pVirtualAllocEx originalVirtualAllocEx = nullptr;
static pVirtualProtectEx originalVirtualProtectEx = nullptr;
static pWriteProcessMemory originalWriteProcessMemory = nullptr;

// advapi32.dll
static pOpenProcessToken originalOpenProcessToken = nullptr;
static pAdjustTokenPrivileges originalAdjustTokenPrivileges = nullptr;
static pImpersonateLoggedOnUser originalImpersonateLoggedOnUser = nullptr;
static pCreateProcessWithTokenW originalCreateProcessWithTokenW = nullptr;

// ntdll.dll
static pNtReadVirtualMemory originalNtReadVirtualMemory = nullptr;
static pNtWriteVirtualMemory originalNtWriteVirtualMemory = nullptr;
static pNtAllocateVirtualMemory originalNtAllocateVirtualMemory = nullptr;

// --- 2. Funções Hookadas ---
static BOOL WINAPI HookedIsDebuggerPresent() {
    LOG("IsDebuggerPresent() foi chamado. Retornando FALSE.");
    return FALSE;
}

static BOOL WINAPI HookedCheckRemoteDebuggerPresent(HANDLE h, PBOOL p) {
    LOG("CheckRemoteDebuggerPresent() foi chamado. Retornando TRUE.");
    if (p) *p = FALSE;
    return TRUE;
}

static HANDLE WINAPI HookedOpenProcess(DWORD a, BOOL b, DWORD p) {
     LOG("OpenProcess() chamado para PID: %lu com Acesso: 0x%X", p, a);
    if (p == GetCurrentProcessId() && (a & PROCESS_ALL_ACCESS)) {
        LOG(">> Acesso total ao processo atual foi BLOQUEADO.");
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }
    return originalOpenProcess(a, b, p);
}

static BOOL WINAPI HookedReadProcessMemory(HANDLE a, LPCVOID b, LPVOID c, SIZE_T d, SIZE_T* e) {
    LOG("ReadProcessMemory() chamado. Endereço: %p, Tamanho: %llu", b, d);
    return originalReadProcessMemory(a, b, c, d, e);
}

static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER p) {
    LOG("SetUnhandledExceptionFilter(new=0x%p) -> FORCING NULL", p);
    return NULL;
}

static BOOL WINAPI HookedVirtualProtect(LPVOID a, SIZE_T b, DWORD c, PDWORD d) {
    LOG("VirtualProtect() chamado. Endereço: %p, Tamanho: %llu, Novo Prot: 0x%X", a, b, c);
    return originalVirtualProtect(a, b, c, d);
}

static NTSTATUS NTAPI HookedNtQueryInformationProcess(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength) 
{
    LOG("NtQueryInformationProcess() chamado com ProcessInformationClass: %d", (int)ProcessInformationClass);

    switch (ProcessInformationClass) {
        case ProcessDebugPort:
            LOG(">> ProcessDebugPort (7) consultado. Retornando 0 (nenhum debugger).");
            if (ProcessInformationLength == sizeof(HANDLE)) {
                *(PHANDLE)ProcessInformation = (HANDLE)0;
                if (ReturnLength) *ReturnLength = sizeof(HANDLE);
                return 0x00000000;
            }
            break;

        case ProcessDebugFlags:
            LOG(">> ProcessDebugFlags (31) consultado. Retornando 1 (sem debug).");
            if (ProcessInformationLength == sizeof(ULONG)) {
                *(PULONG)ProcessInformation = 1;
                if (ReturnLength) *ReturnLength = sizeof(ULONG);
                return 0x00000000;
            }
            break;

        case ProcessDebugObjectHandle:
            if (ProcessInformationLength == sizeof(HANDLE)) {
                *(PHANDLE)ProcessInformation = NULL;
                if (ReturnLength) *ReturnLength = sizeof(HANDLE);
                return 0x00000000;
            }
            break;
    }

    LOG(">> Encaminhando a chamada para o originalNtQueryInformationProcess.");
    return originalNtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        ProcessInformation,
        ProcessInformationLength,
        ReturnLength
    );
}

static LPVOID WINAPI HookedVirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
    LOG("VirtualAllocEx() chamado. Processo: %p, Tamanho: %llu, Tipo: 0x%X, Proteção: 0x%X", hProcess, dwSize, flAllocationType, flProtect);
    return originalVirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
}

static BOOL WINAPI HookedOpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle) {
    LOG("OpenProcessToken() chamado. Processo: %p, Acesso desejado: 0x%X", ProcessHandle, DesiredAccess);
    return originalOpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle);
}

static NTSTATUS NTAPI HookedNtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T Size, PSIZE_T NumberOfBytesRead) {
    LOG("NtReadVirtualMemory() chamado. Processo: %p, Endereço base: %p, Tamanho: %llu", ProcessHandle, BaseAddress, Size);
    return originalNtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, NumberOfBytesRead);
}

static BOOL WINAPI HookedWriteProcessMemory(HANDLE h, LPVOID base, LPCVOID buffer, SIZE_T size, SIZE_T* written) {
    LOG("WriteProcessMemory() chamado. Processo: %p, Endereço destino: %p, Tamanho: %llu", h, base, size);
    return originalWriteProcessMemory(h, base, buffer, size, written);
}

static BOOL WINAPI HookedVirtualProtectEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect) {
    LOG("VirtualProtectEx() chamado. Processo: %p, Endereço: %p, Tamanho: %llu, Novo Prot: 0x%X", hProcess, lpAddress, dwSize, flNewProtect);
    return originalVirtualProtectEx(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
}

static BOOL WINAPI HookedAdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength) {
    LOG("AdjustTokenPrivileges() chamado. Token: %p, DisableAll: %d", TokenHandle, DisableAllPrivileges);
    return originalAdjustTokenPrivileges(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
}

static BOOL WINAPI HookedImpersonateLoggedOnUser(HANDLE hToken) {
    LOG("ImpersonateLoggedOnUser() chamado. Token: %p", hToken);
    return originalImpersonateLoggedOnUser(hToken);
}

static BOOL WINAPI HookedCreateProcessWithTokenW(HANDLE hToken, DWORD dwLogonFlags, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    LOG("CreateProcessWithTokenW() chamado. App: %ls, CmdLine: %ls", lpApplicationName, lpCommandLine);
    return originalCreateProcessWithTokenW(hToken, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

static NTSTATUS NTAPI HookedNtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten) {
    LOG("NtWriteVirtualMemory() chamado. Processo: %p, Endereço: %p, Tamanho: %llu", ProcessHandle, BaseAddress, NumberOfBytesToWrite);
    return originalNtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
}

static NTSTATUS NTAPI HookedNtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect) {
    LOG("NtAllocateVirtualMemory() chamado. Processo: %p, Tamanho: %llu, Tipo: 0x%X, Proteção: 0x%X", ProcessHandle, *RegionSize, AllocationType, Protect);
    return originalNtAllocateVirtualMemory(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

// --- 3. DllMain ---
// Ao final do seu código, antes do DllMain:

DWORD WINAPI InitHookThread(LPVOID) {
    logger::Init();
    LOG("Logger inicializado. Iniciando hooks...");

    if (MH_Initialize() != MH_OK) {
        LOG("MH_Initialize() falhou!");
        return FALSE;
    }

    // Helper macro para criar hook com segurança
    #define CREATE_HOOK(module, func, hook, original)                        \
        if (MH_CreateHookApi(module, func, reinterpret_cast<LPVOID>(hook),  \
            reinterpret_cast<LPVOID*>(&original)) != MH_OK) return FALSE;

    // kernel32.dll hooks
    CREATE_HOOK(L"kernel32.dll", "IsDebuggerPresent", HookedIsDebuggerPresent, originalIsDebuggerPresent);
    CREATE_HOOK(L"kernel32.dll", "CheckRemoteDebuggerPresent", HookedCheckRemoteDebuggerPresent, originalCheckRemoteDebuggerPresent);
    CREATE_HOOK(L"kernel32.dll", "OpenProcess", HookedOpenProcess, originalOpenProcess);
    CREATE_HOOK(L"kernel32.dll", "ReadProcessMemory", HookedReadProcessMemory, originalReadProcessMemory);
    CREATE_HOOK(L"kernel32.dll", "SetUnhandledExceptionFilter", HookedSetUnhandledExceptionFilter, originalSetUnhandledExceptionFilter);
    CREATE_HOOK(L"kernel32.dll", "VirtualProtect", HookedVirtualProtect, originalVirtualProtect);
    CREATE_HOOK(L"kernel32.dll", "VirtualAllocEx", HookedVirtualAllocEx, originalVirtualAllocEx);
    CREATE_HOOK(L"kernel32.dll", "WriteProcessMemory", HookedWriteProcessMemory, originalWriteProcessMemory);
    CREATE_HOOK(L"kernel32.dll", "VirtualProtectEx", HookedVirtualProtectEx, originalVirtualProtectEx);

    // advapi32.dll hooks
    CREATE_HOOK(L"advapi32.dll", "OpenProcessToken", HookedOpenProcessToken, originalOpenProcessToken);
    CREATE_HOOK(L"advapi32.dll", "AdjustTokenPrivileges", HookedAdjustTokenPrivileges, originalAdjustTokenPrivileges);
    CREATE_HOOK(L"advapi32.dll", "ImpersonateLoggedOnUser", HookedImpersonateLoggedOnUser, originalImpersonateLoggedOnUser);
    CREATE_HOOK(L"advapi32.dll", "CreateProcessWithTokenW", HookedCreateProcessWithTokenW, originalCreateProcessWithTokenW);

    // ntdll.dll hooks (manuais, pois MinHook não suporta GetProcAddress + CreateHookApi juntos para ntdll)
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        #define CREATE_HOOK_RAW(name, hook, original)                                \
            do {                                                                     \
                void* fn = reinterpret_cast<void*>(GetProcAddress(ntdll, name));     \
                if (fn &&                                                            \
                    MH_CreateHook(fn, reinterpret_cast<LPVOID>(hook),                \
                                  reinterpret_cast<LPVOID*>(&original)) != MH_OK)    \
                    return FALSE;                                                    \
            } while (0)

        CREATE_HOOK_RAW("NtQueryInformationProcess", HookedNtQueryInformationProcess, originalNtQueryInformationProcess);
        CREATE_HOOK_RAW("NtReadVirtualMemory", HookedNtReadVirtualMemory, originalNtReadVirtualMemory);
        CREATE_HOOK_RAW("NtWriteVirtualMemory", HookedNtWriteVirtualMemory, originalNtWriteVirtualMemory);
        CREATE_HOOK_RAW("NtAllocateVirtualMemory", HookedNtAllocateVirtualMemory, originalNtAllocateVirtualMemory);
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        LOG("MH_EnableHook(MH_ALL_HOOKS) falhou!");
        return FALSE;
    }

    LOG("Todos os hooks foram ativados com sucesso.");
    return TRUE;
}

void UninstallAllHooks() {
    LOG("Desinstalando hooks e fechando o logger.");
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    logger::Close();
}

// Em vez de tudo dentro do DllMain:
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            if (hModule)
                DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0, InitHookThread, nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            UninstallAllHooks();
            break;
    }
    return TRUE;
}