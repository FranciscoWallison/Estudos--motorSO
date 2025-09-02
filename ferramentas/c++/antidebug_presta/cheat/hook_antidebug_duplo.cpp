#include <windows.h>
#include "MinHook.h"

// --- 1. Typedefs e Ponteiros Originais ---
// Funções já existentes
using pIsDebuggerPresent = BOOL(WINAPI*)();
using pCheckRemoteDebuggerPresent = BOOL(WINAPI*)(HANDLE, PBOOL);

// NOVAS FUNÇÕES
using pOpenProcess = HANDLE(WINAPI*)(DWORD, BOOL, DWORD);
using pReadProcessMemory = BOOL(WINAPI*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
using pSetUnhandledExceptionFilter = LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI*)(LPTOP_LEVEL_EXCEPTION_FILTER);
using pVirtualProtect = BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD);

// Ponteiros para as funções originais
static pIsDebuggerPresent originalIsDebuggerPresent = nullptr;
static pCheckRemoteDebuggerPresent originalCheckRemoteDebuggerPresent = nullptr;
// NOVOS PONTEIROS
static pOpenProcess originalOpenProcess = nullptr;
static pReadProcessMemory originalReadProcessMemory = nullptr;
static pSetUnhandledExceptionFilter originalSetUnhandledExceptionFilter = nullptr;
static pVirtualProtect originalVirtualProtect = nullptr;


// --- 2. Nossas Funções Falsas (Hooks) ---

// Hooks já existentes
static BOOL WINAPI HookedIsDebuggerPresent() {
    return FALSE; // Mente
}

static BOOL WINAPI HookedCheckRemoteDebuggerPresent(HANDLE hProcess, PBOOL pbDebuggerPresent) {
    if (pbDebuggerPresent) {
        *pbDebuggerPresent = FALSE; // Mente
    }
    return TRUE;
}

// ===============================================
// NOVAS FUNÇÕES DE HOOK
// ===============================================

static HANDLE WINAPI HookedOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
    // Se o programa tentar abrir a si mesmo com acesso total (típico de um debugger), negamos.
    if (dwProcessId == GetCurrentProcessId() && (dwDesiredAccess & PROCESS_ALL_ACCESS)) {
        // Retorna um handle inválido (NULL) e define um código de erro de "Acesso Negado".
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }
    // Para todas as outras chamadas, permite que a função original execute.
    return originalOpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
}

static BOOL WINAPI HookedReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead) {
    // Hook "passa-através" (pass-through).
    // Por enquanto, apenas chamamos a função original.
    // Isso é mais seguro para não quebrar o programa. No futuro, você poderia adicionar
    // lógica aqui para, por exemplo, esconder partes da memória.
    return originalReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) {
    // Neutraliza a tentativa de definir um filtro de exceção para detectar o debugger.
    // Simplesmente não fazemos nada e retornamos NULL, como se a função tivesse falhado.
    return NULL;
}

static BOOL WINAPI HookedVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect) {
    // Hook "passa-através" (pass-through).
    // Apenas chama a função original. Importante para a estabilidade do programa.
    // Você poderia adicionar logs aqui para ver quais áreas de memória estão sendo modificadas.
    return originalVirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
}


// --- 3. Ponto de Entrada da DLL (DllMain) ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hModule);

            if (MH_Initialize() != MH_OK) {
                return FALSE;
            }

            // --- Lista de hooks para criar ---
            // A MinHook usa o nome da função (string) para encontrá-la na DLL especificada.


            if (MH_CreateHookApi(L"kernel32.dll", "IsDebuggerPresent", reinterpret_cast<LPVOID>(&HookedIsDebuggerPresent), reinterpret_cast<LPVOID*>(&originalIsDebuggerPresent)) != MH_OK) { MH_Uninitialize(); return FALSE; }
            if (MH_CreateHookApi(L"kernel32.dll", "CheckRemoteDebuggerPresent", reinterpret_cast<LPVOID>(&HookedCheckRemoteDebuggerPresent), reinterpret_cast<LPVOID*>(&originalCheckRemoteDebuggerPresent)) != MH_OK) { MH_Uninitialize(); return FALSE; }

            if (MH_CreateHookApi(L"kernel32.dll", "OpenProcess", reinterpret_cast<LPVOID>(&HookedOpenProcess), reinterpret_cast<LPVOID*>(&originalOpenProcess)) != MH_OK) { MH_Uninitialize(); return FALSE; }
            if (MH_CreateHookApi(L"kernel32.dll", "ReadProcessMemory", reinterpret_cast<LPVOID>(&HookedReadProcessMemory), reinterpret_cast<LPVOID*>(&originalReadProcessMemory)) != MH_OK) { MH_Uninitialize(); return FALSE; }
            if (MH_CreateHookApi(L"kernel32.dll", "SetUnhandledExceptionFilter", reinterpret_cast<LPVOID>(&HookedSetUnhandledExceptionFilter), reinterpret_cast<LPVOID*>(&originalSetUnhandledExceptionFilter)) != MH_OK) { MH_Uninitialize(); return FALSE; }
            if (MH_CreateHookApi(L"kernel32.dll", "VirtualProtect", reinterpret_cast<LPVOID>(&HookedVirtualProtect), reinterpret_cast<LPVOID*>(&originalVirtualProtect)) != MH_OK) { MH_Uninitialize(); return FALSE; }


            // Ativa TODOS os hooks criados de uma só vez.
            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
                MH_Uninitialize();
                return FALSE;
            }

            break;
        }
        case DLL_PROCESS_DETACH: {
            // Desativa e limpa tudo de forma segura quando o programa fecha.
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            break;
        }
    }
    return TRUE;
}