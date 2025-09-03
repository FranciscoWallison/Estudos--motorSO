#include <windows.h>
#include <tlhelp32.h>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include "MinHook.h"
#include "logger.h"
#include <winternl.h>

// =========================
//   Globais / Sincronismo
// =========================
static CRITICAL_SECTION g_cs; // protege g_blockedPids
std::set<DWORD> g_blockedPids;

// padronize tudo em minúsculas
std::set<std::wstring> g_blockedWindowNames; // termos (minúsculos) p/ título/classe

// =========================
//   Typedefs / Originals
// =========================
// kernel32.dll
using pIsDebuggerPresent            = BOOL (WINAPI*)();
using pCheckRemoteDebuggerPresent   = BOOL (WINAPI*)(HANDLE, PBOOL);
using pOpenProcess                  = HANDLE (WINAPI*)(DWORD, BOOL, DWORD);
using pSetUnhandledExceptionFilter  = LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI*)(LPTOP_LEVEL_EXCEPTION_FILTER);
using pVirtualProtect               = BOOL (WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD);
using pVirtualAllocEx               = LPVOID (WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
using pVirtualProtectEx             = BOOL (WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, PDWORD);
using pWriteProcessMemory           = BOOL (WINAPI*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
using pCreateThread                 = HANDLE (WINAPI*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
using pResumeThread                 = DWORD (WINAPI*)(HANDLE);
using pExitProcess                  = VOID (WINAPI*)(UINT);
using pTerminateProcess             = BOOL (WINAPI*)(HANDLE, UINT);
using pDebugActiveProcess           = BOOL (WINAPI*)(DWORD);
using pReadProcessMemory            = BOOL (WINAPI*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
using pVirtualQuery                 = SIZE_T (WINAPI*)(LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);
using pCreateRemoteThread           = HANDLE (WINAPI*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);

// user32.dll
using pFindWindowW = HWND (WINAPI*)(LPCWSTR, LPCWSTR);
using pEnumWindows = BOOL (WINAPI*)(WNDENUMPROC, LPARAM);

// advapi32.dll
using pOpenProcessToken         = BOOL (WINAPI*)(HANDLE, DWORD, PHANDLE);
using pAdjustTokenPrivileges    = BOOL (WINAPI*)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

// ntdll.dll
#ifndef NTSTATUS
using NTSTATUS = LONG;
#endif
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

using pNtWriteVirtualMemory = NTSTATUS (NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
using pNtAllocateVirtualMemory = NTSTATUS (NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
using pNtSetInformationThread = NTSTATUS (NTAPI*)(HANDLE, THREADINFOCLASS, PVOID, ULONG);
// >>> IMPORTANTE: evite enum custom de MEMORY_INFORMATION_CLASS; use ULONG para compatibilidade
using pNtQueryVirtualMemory   = NTSTATUS (NTAPI*)(HANDLE, PVOID, ULONG /*MemoryInformationClass*/, PVOID, SIZE_T, PSIZE_T);
using pNtReadVirtualMemory    = NTSTATUS (NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
using pNtProtectVirtualMemory = NTSTATUS (NTAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
using pNtCreateThreadEx       = NTSTATUS (NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, HANDLE, LPTHREAD_START_ROUTINE, LPVOID, ULONG, ULONG_PTR, SIZE_T, SIZE_T, LPVOID);
using pNtOpenProcess          = NTSTATUS (NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);
using pNtQueueApcThread       = NTSTATUS (NTAPI*)(HANDLE, PVOID, PVOID, PVOID, PVOID);
using pNtTerminateProcess     = NTSTATUS (NTAPI*)(HANDLE, NTSTATUS);
using pNtQueryInformationProcess = NTSTATUS (NTAPI*)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

// originals
static pIsDebuggerPresent             originalIsDebuggerPresent = nullptr;
static pCheckRemoteDebuggerPresent    originalCheckRemoteDebuggerPresent = nullptr;
static pOpenProcess                   originalOpenProcess = nullptr;
static pSetUnhandledExceptionFilter   originalSetUnhandledExceptionFilter = nullptr;
static pVirtualProtect                originalVirtualProtect = nullptr;
static pVirtualAllocEx                originalVirtualAllocEx = nullptr;
static pVirtualProtectEx              originalVirtualProtectEx = nullptr;
static pWriteProcessMemory            originalWriteProcessMemory = nullptr;
static pCreateThread                  originalCreateThread = nullptr;
static pResumeThread                  originalResumeThread = nullptr;
static pExitProcess                   originalExitProcess = nullptr;
static pTerminateProcess              originalTerminateProcess = nullptr;
static pDebugActiveProcess            originalDebugActiveProcess = nullptr;
static pReadProcessMemory             originalReadProcessMemory = nullptr;
static pVirtualQuery                  originalVirtualQuery = nullptr;
static pCreateRemoteThread            originalCreateRemoteThread = nullptr;

static pFindWindowW                   originalFindWindowW = nullptr;
static pEnumWindows                   originalEnumWindows = nullptr;

static pOpenProcessToken              originalOpenProcessToken = nullptr;
static pAdjustTokenPrivileges         originalAdjustTokenPrivileges = nullptr;

static pNtQueryInformationProcess     originalNtQueryInformationProcess = nullptr;
static pNtWriteVirtualMemory          originalNtWriteVirtualMemory = nullptr;
static pNtAllocateVirtualMemory       originalNtAllocateVirtualMemory = nullptr;
static pNtSetInformationThread        originalNtSetInformationThread = nullptr;
static pNtReadVirtualMemory           originalNtReadVirtualMemory = nullptr;
static pNtProtectVirtualMemory        originalNtProtectVirtualMemory = nullptr;
static pNtCreateThreadEx              originalNtCreateThreadEx = nullptr;
static pNtOpenProcess                 originalNtOpenProcess = nullptr;
static pNtQueueApcThread              originalNtQueueApcThread = nullptr;
static pNtQueryVirtualMemory          originalNtQueryVirtualMemory = nullptr;
static pNtTerminateProcess            originalNtTerminateProcess = nullptr;

// =========================
//   Helpers
// =========================
static std::wstring to_lower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

static bool ci_contains(const std::wstring& hay, const std::wstring& needle) {
    std::wstring h = to_lower(hay);
    std::wstring n = to_lower(needle);
    return h.find(n) != std::wstring::npos;
}

static std::wstring GetExeNameByPid(DWORD pid) {
    std::wstring name;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return name;
    PROCESSENTRY32W pe{ sizeof(pe) };
    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                name = pe.szExeFile;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return name;
}

static bool IsSuspiciousProcessName(const std::wstring& exe) {
    static const std::vector<std::wstring> bad = {
        L"x64dbg.exe", L"x32dbg.exe", L"ollydbg.exe", L"ida.exe", L"ida64.exe",
        L"windbg.exe", L"processhacker.exe", L"procmon.exe", L"procmon64.exe",
        L"cheatengine-x86_64.exe", L"ghidra.exe", L"wireshark.exe"
    };
    std::wstring e = to_lower(exe);
    for (auto& b : bad) if (e == to_lower(b)) return true;
    return false;
}

// =========================
//   Hooks
// =========================
struct EnumWindowsCallbackData {
    WNDENUMPROC originalCallback;
    LPARAM originalLParam;
};

static BOOL CALLBACK FilteringEnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<EnumWindowsCallbackData*>(lParam);

    wchar_t title[256] = L"", cls[256] = L"";
    GetWindowTextW(hwnd, title, 256);
    GetClassNameW(hwnd, cls, 256);

    std::wstring t = to_lower(title), c = to_lower(cls);
    for (const auto& blocked : g_blockedWindowNames) {
        if (ci_contains(t, blocked) || ci_contains(c, blocked)) {
            LOG("EnumWindows: ocultando janela suspeita: '%ls' (classe '%ls')", title, cls);
            return TRUE; // pula esta janela, segue enumeração
        }
    }
    return data->originalCallback(hwnd, data->originalLParam);
}

static BOOL WINAPI HookedEnumWindows(WNDENUMPROC lpEnumFunc, LPARAM lParam) {
    LOG("EnumWindows() chamado; aplicando filtro case-insensitive");
    EnumWindowsCallbackData d{ lpEnumFunc, lParam };
    return originalEnumWindows(FilteringEnumWindowsProc, reinterpret_cast<LPARAM>(&d));
}

static HWND WINAPI HookedFindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) {
    std::wstring t = to_lower(lpWindowName ? lpWindowName : L"");
    std::wstring c = to_lower(lpClassName ? lpClassName : L"");
    for (const auto& blocked : g_blockedWindowNames) {
        if ((!t.empty() && ci_contains(t, blocked)) || (!c.empty() && ci_contains(c, blocked))) {
            LOG("FindWindowW BLOQUEADO (titulo='%ls', classe='%ls')", lpWindowName ? lpWindowName : L"", lpClassName ? lpClassName : L"");
            SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
            return NULL;
        }
    }
    return originalFindWindowW(lpClassName, lpWindowName);
}

static BOOL WINAPI HookedIsDebuggerPresent() {
    LOG("IsDebuggerPresent() -> FALSE");
    return FALSE;
}

static BOOL WINAPI HookedCheckRemoteDebuggerPresent(HANDLE hProcess, PBOOL pbDebuggerPresent) {
    LOG("CheckRemoteDebuggerPresent() -> FALSE");
    if (pbDebuggerPresent) *pbDebuggerPresent = FALSE;
    return TRUE;
}

static HANDLE WINAPI HookedOpenProcess(DWORD access, BOOL inherit, DWORD pid) {
    {
        // atualização dinâmica de PIDs bloqueados
        std::wstring name = GetExeNameByPid(pid);
        if (!name.empty() && IsSuspiciousProcessName(name)) {
            EnterCriticalSection(&g_cs);
            g_blockedPids.insert(pid);
            LeaveCriticalSection(&g_cs);
            LOG("OpenProcess alvo suspeito detectado dinamicamente: %ls (pid=%lu)", name.c_str(), pid);
        }
    }
    EnterCriticalSection(&g_cs);
    bool blocked = g_blockedPids.count(pid) > 0;
    LeaveCriticalSection(&g_cs);

    if (blocked) {
        LOG("OpenProcess BLOQUEADO (pid=%lu)", pid);
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }
    // [ 377070812][T19676] OpenProcess(pid=5980, access=0x101000)
    // LOG("OpenProcess(pid=%lu, access=0x%X)", pid, access);   
    return originalOpenProcess(access, inherit, pid);
}

static NTSTATUS NTAPI HookedNtOpenProcess(PHANDLE ph, ACCESS_MASK acc, POBJECT_ATTRIBUTES oa, PCLIENT_ID cid) {
    DWORD pid = (cid && cid->UniqueProcess) ? (DWORD)(ULONG_PTR)cid->UniqueProcess : 0;
    if (pid) {
        std::wstring name = GetExeNameByPid(pid);
        if (!name.empty() && IsSuspiciousProcessName(name)) {
            EnterCriticalSection(&g_cs);
            g_blockedPids.insert(pid);
            LeaveCriticalSection(&g_cs);
            LOG("NtOpenProcess alvo suspeito detectado dinamicamente: %ls (pid=%lu)", name.c_str(), pid);
        }
    }
    EnterCriticalSection(&g_cs);
    bool blocked = pid && g_blockedPids.count(pid);
    LeaveCriticalSection(&g_cs);

    if (blocked) {
        if (ph) *ph = NULL;
        LOG("NtOpenProcess BLOQUEADO (pid=%lu) -> STATUS_ACCESS_DENIED", pid);
        return (NTSTATUS)0xC0000022; // STATUS_ACCESS_DENIED
    }
    return originalNtOpenProcess(ph, acc, oa, cid);
}

static BOOL WINAPI HookedReadProcessMemory(HANDLE hProcess, LPCVOID addr, LPVOID buf, SIZE_T n, SIZE_T* read) {
    static unsigned c = 0;
    if ((++c & 0x3F) == 0) // 1 a cada 64
        LOG("ReadProcessMemory(proc=%p, addr=%p, size=%llu)", hProcess, addr, (unsigned long long)n);
    return originalReadProcessMemory(hProcess, addr, buf, n, read);
}


static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER f
) {
    LOG("SetUnhandledExceptionFilter(new=%p) [pass-through]", f);
    // Deixe o app instalar o handler: evita loops/exceções não tratadas
    return originalSetUnhandledExceptionFilter(f);
}


static BOOL WINAPI HookedVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect) {
    
    DWORD modifiableProtect = flNewProtect;

    // Verificamos a permissão solicitada e forçamos a adição de permissão de escrita
    // Isso é feito para garantir que possamos modificar a memória para análise.
    if (!(flNewProtect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_WRITECOPY))) {
        // Se a página é executável, adicione escrita para torná-la EXECUTE_READWRITE
        if (flNewProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ)) {
            modifiableProtect = PAGE_EXECUTE_READWRITE;
        } 
        // Se for apenas de leitura, adicione escrita para torná-la READWRITE
        else if (flNewProtect & PAGE_READONLY) {
            modifiableProtect = PAGE_READWRITE;
        }
        // Em outros casos, podemos tentar adicionar a flag PAGE_WRITECOPY
        else {
             modifiableProtect |= PAGE_WRITECOPY;
        }

        LOG("!!! VirtualProtect: FORÇANDO PERMISSÃO DE ESCRITA. Original: 0x%X, Modificado: 0x%X", flNewProtect, modifiableProtect);
    }

    return originalVirtualProtect(lpAddress, dwSize, modifiableProtect, lpflOldProtect);
}

static BOOL WINAPI HookedVirtualProtectEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect) {

    DWORD modifiableProtect = flNewProtect;

    if (!(flNewProtect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_WRITECOPY))) {
        if (flNewProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ)) {
            modifiableProtect = PAGE_EXECUTE_READWRITE;
        } else if (flNewProtect & PAGE_READONLY) {
            modifiableProtect = PAGE_READWRITE;
        } else {
            modifiableProtect |= PAGE_WRITECOPY;
        }

        LOG("!!! VirtualProtectEx: FORÇANDO PERMISSÃO DE ESCRITA. Original: 0x%X, Modificado: 0x%X", flNewProtect, modifiableProtect);
    }
    
    return originalVirtualProtectEx(hProcess, lpAddress, dwSize, modifiableProtect, lpflOldProtect);
}

static SIZE_T WINAPI HookedVirtualQuery(LPCVOID addr, PMEMORY_BASIC_INFORMATION mbi, SIZE_T len) {
    static unsigned q = 0;
    SIZE_T r = originalVirtualQuery(addr, mbi, len);
    if (r && mbi && (mbi->Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        if ((++q & 0x3F) == 0) // 1/64 chamadas
            LOG("VirtualQuery EXEC region base=%p size=%llu", mbi->BaseAddress, (unsigned long long)mbi->RegionSize);
    }
    return r;
}

static HANDLE WINAPI HookedCreateRemoteThread(HANDLE hProcess, LPSECURITY_ATTRIBUTES a, SIZE_T st, LPTHREAD_START_ROUTINE start, LPVOID param, DWORD flags, LPDWORD tid) {
    LOG("CreateRemoteThread alvo=%p start=%p flags=0x%X", hProcess, start, flags);
    return originalCreateRemoteThread(hProcess, a, st, start, param, flags, tid);
}

static NTSTATUS NTAPI HookedNtCreateThreadEx(PHANDLE th, ACCESS_MASK da, POBJECT_ATTRIBUTES oa, HANDLE proc, LPTHREAD_START_ROUTINE start, PVOID arg, ULONG cf, ULONG_PTR zb, SIZE_T ss, SIZE_T mss, PVOID attrs) {
    DWORD pid = GetProcessId(proc);
    if (pid != GetCurrentProcessId())
        LOG("NtCreateThreadEx REMOTA no pid=%lu start=%p", pid, start);
    return originalNtCreateThreadEx(th, da, oa, proc, start, arg, cf, zb, ss, mss, attrs);
}

static DWORD WINAPI HookedResumeThread(HANDLE hThread) {
    LOG("ResumeThread(thread=%p)", hThread);
    return originalResumeThread(hThread);
}

static VOID WINAPI HookedExitProcess(UINT code) {
    LOG("ExitProcess(code=%u)", code);
    originalExitProcess(code);
}

static BOOL WINAPI HookedTerminateProcess(HANDLE hProc, UINT code) {
    // Se o handle for o do processo atual, bloqueie a chamada
    if (hProc == GetCurrentProcess()) {
        LOG("!!! BLOQUEADO: Tentativa de TerminateProcess no próprio processo (code=%u)", code);
        return TRUE; // Minta, dizendo que o processo foi terminado com sucesso
    }
    LOG("TerminateProcess(proc=%p, code=%u)", hProc, code);
    return originalTerminateProcess(hProc, code);
}

static NTSTATUS NTAPI HookedNtTerminateProcess(HANDLE hProc, NTSTATUS status) {
    // Se o handle for o do processo atual, bloqueie a chamada
    if (hProc == GetCurrentProcess()) {
        LOG("!!! BLOQUEADO: Tentativa de NtTerminateProcess no próprio processo (status=0x%08X)", (unsigned)status);
        return STATUS_SUCCESS; // Minta, dizendo que a operação foi bem-sucedida
    }
    LOG("NtTerminateProcess(proc=%p, status=0x%08X)", hProc, (unsigned)status);
    return originalNtTerminateProcess(hProc, status);
}

// Self-debug: preferir falso + errno coerente, para não deixar o app esperando eventos de debug inexistentes
static BOOL WINAPI HookedDebugActiveProcess(DWORD pid) {
    if (pid == GetCurrentProcessId()) {
        LOG("DebugActiveProcess(self) BLOQUEADO -> FALSE/ACCESS_DENIED");
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return originalDebugActiveProcess(pid);
}

static NTSTATUS NTAPI HookedNtSetInformationThread(HANDLE th, THREADINFOCLASS cls, PVOID info, ULONG len) {
    if (cls == (THREADINFOCLASS)17 /*ThreadHideFromDebugger*/) {
        LOG("NtSetInformationThread(ThreadHideFromDebugger) -> simulando SUCESSO");
        return STATUS_SUCCESS;
    }
    return originalNtSetInformationThread(th, cls, info, len);
}

// NtQueryInformationProcess com melhorias (23/37/43 + throttle)
static NTSTATUS NTAPI HookedNtQueryInformationProcess(HANDLE proc, PROCESSINFOCLASS pic, PVOID info, ULONG infolen, PULONG retlen) {
    if (pic == (PROCESSINFOCLASS)0 /*ProcessBasicInformation*/) {
        static int c0 = 0;
        if ((++c0 % 1000) == 0) {
            LOG("NtQueryInformationProcess [throttled] class=0 (count=%d)", c0);
        }
    } else if (pic == (PROCESSINFOCLASS)23) {
        static int c23 = 0;
        if ((++c23 % 1000) == 0) {
            LOG("NtQueryInformationProcess [throttled] class=23 (count=%d)", c23);
        }
    } else {
        LOG("NtQueryInformationProcess class=%d", (int)pic);
    }

    switch ((int)pic) {
        case 7: { // ProcessDebugPort
            if (info && infolen >= sizeof(HANDLE)) {
                *(PHANDLE)info = (HANDLE)0;
                if (retlen) *retlen = sizeof(HANDLE);
                return STATUS_SUCCESS;
            }
            break;
        }
        case 30: { // ProcessDebugObjectHandle
            // Retornar "não configurado" é mais seguro que NULL com sucesso às vezes
            return (NTSTATUS)0xC0000353; // STATUS_PORT_NOT_SET
        }
        case 31: { // ProcessDebugFlags
            if (info && infolen >= sizeof(ULONG)) {
                *(PULONG)info = 1; // "NoDebugInherit"
                if (retlen) *retlen = sizeof(ULONG);
                return STATUS_SUCCESS;
            }
            break;
        }
        case 26: { // ProcessWow64Information
            if (info && infolen >= sizeof(ULONG_PTR)) {
                *(PULONG_PTR)info = 0; // tratar como nativo
                if (retlen) *retlen = sizeof(ULONG_PTR);
                return STATUS_SUCCESS;
            }
            break;
        }
        case 37: { // ProcessCookie (varia por SDK; neutraliza)
            if (info && infolen >= sizeof(ULONG_PTR)) {
                *(PULONG_PTR)info = 0;
                if (retlen) *retlen = sizeof(ULONG_PTR);
                return STATUS_SUCCESS;
            }
            break;
        }
        case 43: { // ProcessDebugProtection (varia por SDK)
            if (info && infolen >= sizeof(ULONG)) {
                *(PULONG)info = 0; // sem proteção especial
                if (retlen) *retlen = sizeof(ULONG);
                return STATUS_SUCCESS;
            }
            break;
        }
    }
    return originalNtQueryInformationProcess(proc, pic, info, infolen, retlen);
}

static LPVOID WINAPI HookedVirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
    LOG("VirtualAllocEx() chamado. Processo: %p, Tamanho: %llu, Tipo: 0x%X, Proteção: 0x%X", hProcess, dwSize, flAllocationType, flProtect);
    return originalVirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
}

static BOOL WINAPI HookedWriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten) {
    LOG("WriteProcessMemory() chamado. Processo: %p, Endereço destino: %p, Tamanho: %llu", hProcess, lpBaseAddress, nSize);
    return originalWriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

static HANDLE WINAPI HookedCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    if (dwCreationFlags & CREATE_SUSPENDED) {
        LOG("!!! ALERTA: CreateThread chamada com a flag CREATE_SUSPENDED (0x4). Endereço de início: %p", lpStartAddress);
    } else {
        LOG("CreateThread chamada. Endereço de início: %p", lpStartAddress);
    }
    HANDLE hThread = originalCreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    if (hThread && (dwCreationFlags & CREATE_SUSPENDED)) {
        LOG("!!! THREAD SUSPEITA CRIADA! Handle: %p, ID: %lu", hThread, (lpThreadId ? *lpThreadId : 0));
    }
    return hThread;
}


static NTSTATUS NTAPI HookedNtQueryVirtualMemory(HANDLE proc, PVOID base, ULONG mic, PVOID mem, SIZE_T len, PSIZE_T out) {
    // só log básico para não poluir
    return originalNtQueryVirtualMemory(proc, base, mic, mem, len, out);
}

static BOOL WINAPI HookedOpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle) {
    if (DesiredAccess & TOKEN_ADJUST_PRIVILEGES)
        LOG("OpenProcessToken com TOKEN_ADJUST_PRIVILEGES");
    return originalOpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle);
}

static BOOL WINAPI HookedAdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength) {
    if (NewState && NewState->PrivilegeCount) {
        for (DWORD i=0;i<NewState->PrivilegeCount;i++) {
            const LUID_AND_ATTRIBUTES& laa = NewState->Privileges[i];
            wchar_t name[256]; DWORD sz = 256;
            LUID luid = laa.Luid;
            if (LookupPrivilegeNameW(nullptr, &luid, name, &sz)) {

                bool en = (laa.Attributes & SE_PRIVILEGE_ENABLED) != 0;
                LOG("AdjustTokenPrivileges: %ls -> %s", name, en ? "ENABLED" : "disabled");
                if (en && wcscmp(name, L"SeDebugPrivilege") == 0)
                    LOG("!!! ALERTA: SeDebugPrivilege ATIVADO");
            }
        }
    }
    return originalAdjustTokenPrivileges(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
}


static NTSTATUS NTAPI HookedNtReadVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToRead,
    PSIZE_T NumberOfBytesRead
) {
    // contador por segundo (leve)
    static ULONGLONG lastTick = 0;
    static ULONG readsInWindow = 0;

    ULONGLONG now = GetTickCount64();
    if (now - lastTick >= 1000) {
        // resumo 1x/s (evita poluir)
        LOG("NtReadVirtualMemory rate: %lu reads/s", readsInWindow);
        lastTick = now;
        readsInWindow = 0;
    }
    ++readsInWindow;

    // throttle de log:
    const BOOL isSelf =
        (ProcessHandle == GetCurrentProcess()) ||
        (ProcessHandle == (HANDLE)(ULONG_PTR)-1);

    // Regra: só loga se NÃO for self **e**
    //   - leitura grande (>= 0x1000), ou
    //   - 1 a cada 64 chamadas
    if (!isSelf && (NumberOfBytesToRead >= 0x1000 || ((readsInWindow & 0x3F) == 0))) {
        LOG("NtReadVirtualMemory proc=%p addr=%p size=%llu",
            ProcessHandle, BaseAddress, (unsigned long long)NumberOfBytesToRead);
    }

    return originalNtReadVirtualMemory(
        ProcessHandle, BaseAddress, Buffer,
        NumberOfBytesToRead, NumberOfBytesRead
    );
}

static NTSTATUS NTAPI HookedNtQueueApcThread(HANDLE ThreadHandle, PVOID ApcRoutine, PVOID ApcArgument1, PVOID ApcArgument2, PVOID ApcArgument3) {
    LOG("!!! ALERTA DE INJEÇÃO APC: NtQueueApcThread chamado para a thread handle %p. Rotina: %p", ThreadHandle, ApcRoutine);
    return originalNtQueueApcThread(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
}

static NTSTATUS NTAPI HookedNtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten) {
    LOG("NtWriteVirtualMemory() chamado. Processo: %p, Endereço: %p, Tamanho: %llu", ProcessHandle, BaseAddress, NumberOfBytesToWrite);
    return originalNtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
}

static NTSTATUS NTAPI HookedNtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect) {
    SIZE_T size = (RegionSize ? *RegionSize : 0);
    LOG("NtAllocateVirtualMemory() chamado. Processo: %p, Tamanho: %llu, Tipo: 0x%X, Proteção: 0x%X", ProcessHandle, size, AllocationType, Protect);
    return originalNtAllocateVirtualMemory(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

static NTSTATUS NTAPI HookedNtProtectVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect) {

    ULONG modifiableProtect = NewProtect;

    if (!(NewProtect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_WRITECOPY))) {
        if (NewProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ)) {
            modifiableProtect = PAGE_EXECUTE_READWRITE;
        } else if (NewProtect & PAGE_READONLY) {
            modifiableProtect = PAGE_READWRITE;
        } else {
            modifiableProtect |= PAGE_WRITECOPY;
        }

        // LOG("!!! NtProtectVirtualMemory: FORÇANDO PERMISSÃO DE ESCRITA. Original: 0x%X, Modificado: 0x%X", NewProtect, modifiableProtect);
    }

    return originalNtProtectVirtualMemory(ProcessHandle, BaseAddress, RegionSize, modifiableProtect, OldProtect);
}

// =========================
//   Init / DllMain
// =========================
#define CREATE_HOOK(module, func, hook, original) \
    if (MH_CreateHookApi(module, func, reinterpret_cast<LPVOID>(hook), reinterpret_cast<LPVOID*>(&(original))) != MH_OK) return FALSE;

DWORD WINAPI InitHookThread(LPVOID) {
    logger::Init();
    InitializeCriticalSection(&g_cs);

    LOG("Logger inicializado. Iniciando hooks...");
    LOG("=========================================================================");
    // semente inicial de PIDs bloqueados
    const wchar_t* blockedProcessNames[] = {
        L"x64dbg.exe", L"x32dbg.exe", L"ollydbg.exe", L"ida.exe", L"ida64.exe",
        L"windbg.exe", L"processhacker.exe", L"procmon.exe", L"procmon64.exe",
        L"cheatengine-x86_64.exe", L"ghidra.exe", L"wireshark.exe"
    };
    for (auto name : blockedProcessNames) {
        // carrega PIDs atuais
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe{ sizeof(pe) };
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, name) == 0) {
                        EnterCriticalSection(&g_cs);
                        g_blockedPids.insert(pe.th32ProcessID);
                        LeaveCriticalSection(&g_cs);
                        LOG("Processo suspeito detectado no start: %ls (pid=%lu)", name, pe.th32ProcessID);
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }
    }
    LOG("=========================================================================");
    // termos de janela/classe (minúsculos)
    g_blockedWindowNames = {
        L"x64dbg", L"x32dbg", L"ollydbg", L"ida", L"windbg", L"process hacker", L"wireshark", L"cheat engine"
    };

    if (MH_Initialize() != MH_OK) {
        LOG("MH_Initialize() falhou!");
        return FALSE;
    }
    LOG("=============================kernel32==================================");
    // kernel32
    CREATE_HOOK(L"kernel32.dll", "IsDebuggerPresent", HookedIsDebuggerPresent, originalIsDebuggerPresent);
    CREATE_HOOK(L"kernel32.dll", "CheckRemoteDebuggerPresent", HookedCheckRemoteDebuggerPresent, originalCheckRemoteDebuggerPresent);
    CREATE_HOOK(L"kernel32.dll", "OpenProcess", HookedOpenProcess, originalOpenProcess);
    CREATE_HOOK(L"kernel32.dll", "SetUnhandledExceptionFilter", HookedSetUnhandledExceptionFilter, originalSetUnhandledExceptionFilter);
    CREATE_HOOK(L"kernel32.dll", "VirtualProtect", HookedVirtualProtect, originalVirtualProtect);
    CREATE_HOOK(L"kernel32.dll", "VirtualAllocEx", HookedVirtualAllocEx, originalVirtualAllocEx);
    CREATE_HOOK(L"kernel32.dll", "WriteProcessMemory", HookedWriteProcessMemory, originalWriteProcessMemory);
    CREATE_HOOK(L"kernel32.dll", "CreateThread", HookedCreateThread, originalCreateThread);
    CREATE_HOOK(L"kernel32.dll", "ResumeThread", HookedResumeThread, originalResumeThread);
    CREATE_HOOK(L"kernel32.dll", "ExitProcess", HookedExitProcess, originalExitProcess);
    CREATE_HOOK(L"kernel32.dll", "TerminateProcess", HookedTerminateProcess, originalTerminateProcess);
    CREATE_HOOK(L"kernel32.dll", "DebugActiveProcess", HookedDebugActiveProcess, originalDebugActiveProcess);
    CREATE_HOOK(L"kernel32.dll", "ReadProcessMemory", HookedReadProcessMemory, originalReadProcessMemory);
    CREATE_HOOK(L"kernel32.dll", "VirtualQuery", HookedVirtualQuery, originalVirtualQuery);
    CREATE_HOOK(L"kernel32.dll", "CreateRemoteThread", HookedCreateRemoteThread, originalCreateRemoteThread);
    CREATE_HOOK(L"kernel32.dll", "VirtualProtectEx", HookedVirtualProtectEx, originalVirtualProtectEx);
    LOG("=============================user32==================================");
    // user32
    CREATE_HOOK(L"user32.dll", "FindWindowW", HookedFindWindowW, originalFindWindowW);
    CREATE_HOOK(L"user32.dll", "EnumWindows", HookedEnumWindows, originalEnumWindows);
    LOG("=============================advapi32==================================");
    // advapi32
    CREATE_HOOK(L"advapi32.dll", "OpenProcessToken", HookedOpenProcessToken, originalOpenProcessToken);
    CREATE_HOOK(L"advapi32.dll", "AdjustTokenPrivileges", HookedAdjustTokenPrivileges, originalAdjustTokenPrivileges);

    // ntdll (raw)
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto CHR = [&](const char* name, LPVOID hook, LPVOID* orig)->BOOL {
            void* fn = reinterpret_cast<void*>(GetProcAddress(ntdll, name));
            if (!fn) return TRUE; // ignora se não existir
            return MH_CreateHook(fn, hook, orig) == MH_OK;
        };
        LOG("=============================ntdll==================================");
        if (!CHR("NtQueryInformationProcess", (LPVOID)HookedNtQueryInformationProcess, (LPVOID*)&originalNtQueryInformationProcess)) return FALSE;
        if (!CHR("NtSetInformationThread",   (LPVOID)HookedNtSetInformationThread,   (LPVOID*)&originalNtSetInformationThread))   return FALSE;
        if (!CHR("NtReadVirtualMemory",      (LPVOID)HookedNtReadVirtualMemory,      (LPVOID*)&originalNtReadVirtualMemory))      return FALSE;
        if (!CHR("NtCreateThreadEx",         (LPVOID)HookedNtCreateThreadEx,         (LPVOID*)&originalNtCreateThreadEx))         return FALSE;
        if (!CHR("NtOpenProcess",            (LPVOID)HookedNtOpenProcess,            (LPVOID*)&originalNtOpenProcess))            return FALSE;
        if (!CHR("NtQueueApcThread",         (LPVOID)HookedNtQueueApcThread,         (LPVOID*)&originalNtQueueApcThread))         return FALSE;
        if (!CHR("NtQueryVirtualMemory",     (LPVOID)HookedNtQueryVirtualMemory,     (LPVOID*)&originalNtQueryVirtualMemory))     return FALSE;
        if (!CHR("NtWriteVirtualMemory",     (LPVOID)HookedNtWriteVirtualMemory,     (LPVOID*)&originalNtWriteVirtualMemory))     return FALSE;
        // if (!CHR("NtAllocateVirtualMemory",  (LPVOID)HookedNtAllocateVirtualMemory,  (LPVOID*)&originalNtAllocateVirtualMemory))  return FALSE;
        if (!CHR("NtProtectVirtualMemory",   (LPVOID)HookedNtProtectVirtualMemory,   (LPVOID*)&originalNtProtectVirtualMemory))   return FALSE;
        if (!CHR("NtTerminateProcess",       (LPVOID)HookedNtTerminateProcess,       (LPVOID*)&originalNtTerminateProcess))       return FALSE;
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
    DeleteCriticalSection(&g_cs);
    logger::Close();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            if (hModule) DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0, InitHookThread, nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            // preferível sinalizar/usar thread, mas vamos limpar rápido aqui
            UninstallAllHooks();
            break;
    }
    return TRUE;
}
