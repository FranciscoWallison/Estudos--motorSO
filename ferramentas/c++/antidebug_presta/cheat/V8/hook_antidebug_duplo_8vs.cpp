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
#include <psapi.h>
#include <map> 
#include <cstring>
// #define SLR_USE_LOG //usa seu LOG em vez de stderr
#include "salker.h"
#include <string_view>
#include <unordered_map>
#include "pe_dump_util.h"
#include <atomic> 
#include <winioctl.h>

// =========================
//   Globais / Sincronismo
// =========================

static CRITICAL_SECTION g_cs;
std::set<DWORD> g_blockedPids;
std::set<std::wstring> g_blockedWindowNames;
static bool g_force_write_globally = false;
static std::map<LPVOID, DWORD> g_activeRwxPages;
#if defined(_MSC_VER)
  #include <intrin.h>
  #define GET_RETURN_ADDRESS() _ReturnAddress()
#elif defined(__GNUC__) || defined(__clang__)
  #ifndef _ReturnAddress
    #define _ReturnAddress() __builtin_return_address(0)
  #endif
  #ifndef _AddressOfReturnAddress
    #define _AddressOfReturnAddress() __builtin_frame_address(0)
  #endif
  #define GET_RETURN_ADDRESS() _ReturnAddress()
#else
  #error "Compilador não suportado para obter o endereço de retorno!"
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Modo: 0 = observar (default), 1 = bypass (spoofar STATUS_SUCCESS)
#ifndef AB_BYPASS_THREADHIDE
#define AB_BYPASS_THREADHIDE 0
#endif

#ifndef ThreadHideFromDebugger
#define ThreadHideFromDebugger ((THREADINFOCLASS)0x11)
#endif


#ifndef DUMP_TOUCH_PERMS
#define DUMP_TOUCH_PERMS 0
#endif

#ifndef ProcessBasicInformation
#define ProcessBasicInformation 0
#endif
#ifndef ProcessImageFileName
#define ProcessImageFileName 27
#endif
#ifndef ProcessImageFileNameWin32
#define ProcessImageFileNameWin32 43 // Win7+
#endif

typedef NTSTATUS (NTAPI *PFN_NtQueryInformationProcess)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);


static HMODULE g_hSelf = nullptr;











struct ProcNameCacheEntry {
    std::wstring name;
    ULONGLONG    lastSeenMs;
};
static CRITICAL_SECTION gProcNameCs;
static bool gProcNameCsInit = false;
typedef struct _PROCESS_BASIC_INFORMATION_MIN {
    PVOID Reserved1;
    PVOID PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION_MIN;
static std::unordered_map<DWORD, ProcNameCacheEntry> gProcNameCache;

static void EnsureProcNameCs(){
    if (!gProcNameCsInit) {
        InitializeCriticalSection(&gProcNameCs);
        gProcNameCsInit = true;
    }
}
static HMODULE   gSelfMod   = nullptr;
static uintptr_t gSelfBase  = 0;
static size_t    gSelfSize  = 0;















struct ShimConfig {
    bool swallow_int3        = false; // true = VEH consome INT3 (NÃO recomendado com ProcSentinela)
    bool swallow_ods         = false; // true = engole OutputDebugStringW
    bool force_write_on_prot = false; // true = força R/W em VirtualProtect(Ex)/NtProtectVirtualMemory
    bool block_self_kill     = true;  // true = bloqueia TerminateProcess/NtTerminateProcess no self
    bool hide_debug_flags    = true;  // true = spoof BeingDebugged / DebugPort / DebugFlags
} g_cfg;

// leitura simples de env var: "1" liga, qualquer outra desliga
static bool env_on(const wchar_t* name) {
    wchar_t buf[8]{0}; DWORD n = GetEnvironmentVariableW(name, buf, 8);
    return n && buf[0] == L'1';
}

// =========================
//   Typedefs / Originals
// =========================

// kernel32.dll
using pIsDebuggerPresent            = BOOL   (WINAPI*)();                                                                     // kernel32.dll
using pCheckRemoteDebuggerPresent   = BOOL   (WINAPI*)(HANDLE, PBOOL);                                                        // kernel32.dll
using pOpenProcess                  = HANDLE (WINAPI*)(DWORD, BOOL, DWORD);                                                   // kernel32.dll
using pCreateToolhelp32Snapshot     = HANDLE (WINAPI*)(DWORD, DWORD);                                                         // kernel32.dll
using pProcess32FirstW              = BOOL   (WINAPI*)(HANDLE, LPPROCESSENTRY32W);                                            // kernel32.dll
using pProcess32NextW               = BOOL   (WINAPI*)(HANDLE, LPPROCESSENTRY32W);                                            // kernel32.dll
using pSetUnhandledExceptionFilter  = LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI*)(LPTOP_LEVEL_EXCEPTION_FILTER);                   // kernel32.dll
using pVirtualProtect               = BOOL   (WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD);                                        // kernel32.dll
using pVirtualAllocEx               = LPVOID (WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);                                 // kernel32.dll
using pVirtualProtectEx             = BOOL   (WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, PDWORD);                                // kernel32.dll
using pWriteProcessMemory           = BOOL   (WINAPI*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);                             // kernel32.dll
using pCreateThread                 = HANDLE (WINAPI*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD); // kernel32.dll
using pResumeThread                 = DWORD  (WINAPI*)(HANDLE);                                                                // kernel32.dll
using pExitProcess                  = VOID   (WINAPI*)(UINT);                                                                  // kernel32.dll
using pTerminateProcess             = BOOL   (WINAPI*)(HANDLE, UINT);                                                          // kernel32.dll
using pDebugActiveProcess           = BOOL   (WINAPI*)(DWORD);                                                                 // kernel32.dll
using pReadProcessMemory            = BOOL   (WINAPI*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);                              // kernel32.dll
using pVirtualQuery                 = SIZE_T (WINAPI*)(LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);                            // kernel32.dll
using pCreateRemoteThread           = HANDLE (WINAPI*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD); // kernel32.dll
using pK32EnumProcesses             = BOOL   (WINAPI*)(DWORD* lpidProcess, DWORD cb, DWORD* lpcbNeeded);                       // kernel32.dll (alias moderno de EnumProcesses)
using pOutputDebugStringW           = VOID (WINAPI*)(LPCWSTR); // kernel32.dll

// -----------------------------------------------------------------------------
// psapi.dll  (API antiga)

// psapi.dll
using pEnumProcesses                = BOOL   (WINAPI*)(DWORD* lpidProcess, DWORD cb, DWORD* lpcbNeeded);                      // psapi.dll

// -----------------------------------------------------------------------------
// user32.dll  (GUI/janelas)

// user32.dll
using pFindWindowW                  = HWND   (WINAPI*)(LPCWSTR, LPCWSTR);                                                     // user32.dll
using pEnumWindows                  = BOOL   (WINAPI*)(WNDENUMPROC, LPARAM);                                                  // user32.dll

// -----------------------------------------------------------------------------
// advapi32.dll  (segurança/privilegios)

// advapi32.dll
using pOpenProcessToken             = BOOL   (WINAPI*)(HANDLE, DWORD, PHANDLE);                                               // advapi32.dll
using pAdjustTokenPrivileges        = BOOL   (WINAPI*)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);    // advapi32.dll

// -----------------------------------------------------------------------------
// ntdll.dll  (NT Native API)

#ifndef NTSTATUS
using NTSTATUS = LONG;
#endif
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

// ntdll.dll
using pNtWriteVirtualMemory         = NTSTATUS (NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);                               // ntdll.dll
using pNtAllocateVirtualMemory      = NTSTATUS (NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);                    // ntdll.dll
using pNtSetInformationThread       = NTSTATUS (NTAPI*)(HANDLE, THREADINFOCLASS, PVOID, ULONG);                               // ntdll.dll
// Dica: para máxima compat, mantenha MemoryInformationClass como ULONG se não tiver enum oficial
using pNtQueryVirtualMemory         = NTSTATUS (NTAPI*)(HANDLE, PVOID, ULONG /*MemoryInformationClass*/, PVOID, SIZE_T, PSIZE_T); // ntdll.dll
using pNtReadVirtualMemory          = NTSTATUS (NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);                               // ntdll.dll
using pNtProtectVirtualMemory       = NTSTATUS (NTAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);                              // ntdll.dll
using pNtCreateThreadEx             = NTSTATUS (NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, HANDLE, PVOID /*Start*/, PVOID, ULONG, ULONG_PTR, SIZE_T, SIZE_T, PVOID); // ntdll.dll
using pNtOpenProcess                = NTSTATUS (NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);                // ntdll.dll
using pNtQueueApcThread             = NTSTATUS (NTAPI*)(HANDLE, PVOID, PVOID, PVOID, PVOID);                                  // ntdll.dll
using pNtTerminateProcess           = NTSTATUS (NTAPI*)(HANDLE, NTSTATUS);                                                    // ntdll.dll
using pNtQueryInformationProcess    = NTSTATUS (NTAPI*)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);                      // ntdll.dll

using pNtQuerySystemInformation = NTSTATUS (NTAPI*)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

// originals
// ========================= kernel32.dll =========================
static pIsDebuggerPresent           originalIsDebuggerPresent           = nullptr; // kernel32.dll
static pCheckRemoteDebuggerPresent  originalCheckRemoteDebuggerPresent  = nullptr; // kernel32.dll
static pOpenProcess                 originalOpenProcess                 = nullptr; // kernel32.dll
static pSetUnhandledExceptionFilter originalSetUnhandledExceptionFilter = nullptr; // kernel32.dll
static pSetUnhandledExceptionFilter originalSetUnhandledExceptionFilterKB = nullptr; // kernelbase.dll

static pVirtualProtect              originalVirtualProtect              = nullptr; // kernel32.dll
static pVirtualProtect              originalVirtualProtectKB            = nullptr; // kernelbase.dll
static pVirtualAllocEx              originalVirtualAllocEx              = nullptr; // kernel32.dll
static pVirtualProtectEx            originalVirtualProtectEx            = nullptr; // kernel32.dll
static pWriteProcessMemory          originalWriteProcessMemory          = nullptr; // kernel32.dll
static pCreateThread                originalCreateThread                = nullptr; // kernel32.dll
static pResumeThread                originalResumeThread                = nullptr; // kernel32.dll
static pExitProcess                 originalExitProcess                 = nullptr; // kernel32.dll
static pTerminateProcess            originalTerminateProcess            = nullptr; // kernel32.dll
static pDebugActiveProcess          originalDebugActiveProcess          = nullptr; // kernel32.dll
static pReadProcessMemory           originalReadProcessMemory           = nullptr; // kernel32.dll
static pReadProcessMemory           originalReadProcessMemoryKB         = nullptr; // kernelbase.dll

static pVirtualQuery                originalVirtualQuery                = nullptr; // kernel32.dll
static pCreateRemoteThread          originalCreateRemoteThread          = nullptr; // kernel32.dll
static pCreateToolhelp32Snapshot    originalCreateToolhelp32Snapshot    = nullptr; // kernel32.dll
static pProcess32FirstW             originalProcess32FirstW             = nullptr; // kernel32.dll
static pProcess32NextW              originalProcess32NextW              = nullptr; // kernel32.dll
static pK32EnumProcesses            originalK32EnumProcesses            = nullptr; // kernel32.dll
static pOutputDebugStringW          originalOutputDebugStringW          = nullptr; // kernel32.dll

// =========================== psapi.dll ==========================
static pEnumProcesses               originalEnumProcesses               = nullptr; // psapi.dll

// =========================== user32.dll =========================
static pFindWindowW                 originalFindWindowW                 = nullptr; // user32.dll
static pEnumWindows                 originalEnumWindows                 = nullptr; // user32.dll

// ========================== advapi32.dll ========================
static pOpenProcessToken            originalOpenProcessToken            = nullptr; // advapi32.dll
static pAdjustTokenPrivileges       originalAdjustTokenPrivileges       = nullptr; // advapi32.dll

// =========================== ntdll.dll ==========================
static pNtQuerySystemInformation    originalNtQuerySystemInformation    = nullptr; // ntdll.dll
static pNtOpenProcess               originalNtOpenProcess               = nullptr; // ntdll.dll
static pNtQueryInformationProcess   originalNtQueryInformationProcess   = nullptr; // ntdll.dll
static pNtWriteVirtualMemory        originalNtWriteVirtualMemory        = nullptr; // ntdll.dll
static pNtAllocateVirtualMemory     originalNtAllocateVirtualMemory     = nullptr; // ntdll.dll
static pNtSetInformationThread      originalNtSetInformationThread      = nullptr; // ntdll.dll
static pNtReadVirtualMemory         originalNtReadVirtualMemory         = nullptr; // ntdll.dll
static pNtProtectVirtualMemory      originalNtProtectVirtualMemory      = nullptr; // ntdll.dll
static pNtCreateThreadEx            originalNtCreateThreadEx            = nullptr; // ntdll.dll
static pNtQueueApcThread            originalNtQueueApcThread            = nullptr; // ntdll.dll
static pNtQueryVirtualMemory        originalNtQueryVirtualMemory        = nullptr; // ntdll.dll
static pNtTerminateProcess          originalNtTerminateProcess          = nullptr; // ntdll.dll

// =========================
//   Helpers
// =========================
static inline BOOL CallOrig_VirtualProtect(LPVOID a, SIZE_T s, DWORD np, PDWORD op) {
    if (originalVirtualProtectKB)  return originalVirtualProtectKB(a, s, np, op);
    if (originalVirtualProtect)    return originalVirtualProtect(a, s, np, op);
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static inline BOOL CallOrig_ReadProcessMemory(HANDLE p, LPCVOID ba, LPVOID buf, SIZE_T n, SIZE_T* got) {
    if (originalReadProcessMemoryKB) return originalReadProcessMemoryKB(p, ba, buf, n, got);
    if (originalReadProcessMemory)   return originalReadProcessMemory(p, ba, buf, n, got);
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

// Converte ponteiro de FUNÇÃO para um valor "endereçável" (para logs/IsFromSelf/GetModule*)
template <class F>
static inline const void* FADDR(F f) {
    return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(f));
}


static inline bool IsFromSelf(const void* addr) {
    const uintptr_t a = (uintptr_t)addr;
    return gSelfBase && (a >= gSelfBase) && (a < (gSelfBase + gSelfSize));
}

static std::wstring ToLower(std::wstring s){
    for (auto &ch : s) ch = (wchar_t)towlower(ch);
    return s;
}
static bool ci_contains(const std::wstring& hay, const std::wstring& needle) {
    std::wstring h = ToLower(hay);
    std::wstring n = ToLower(needle);
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

inline bool equals_icase(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) return false;
    }
    return true;
}

inline std::wstring_view basename_ws(std::wstring_view p) {
    size_t pos = p.find_last_of(L"\\/");
    return (pos == std::wstring_view::npos) ? p : p.substr(pos + 1);
}

inline bool IsSecurityProcessName(std::wstring_view exeOrPath) {
 
    std::wstring_view exe = basename_ws(exeOrPath);
    static constexpr std::wstring_view kSecProcs[] = {
        L"x64dbg.exe", L"x32dbg.exe", L"ollydbg.exe", L"ida.exe", L"ida64.exe",
        L"windbg.exe", L"processhacker.exe", L"procmon.exe", L"procmon64.exe",
        L"cheatengine-x86_64.exe", L"gggggseguro-x86_64.exe", L"wireshark.exe"
    };
    for (auto s : kSecProcs) {
        if (equals_icase(exe, s)) return true;
    }
    return false;
}

static std::wstring GetProcessModulePath(HANDLE hProcess) {
    if (hProcess == nullptr) return L"";
    wchar_t path[MAX_PATH] = { 0 };
    if (GetModuleFileNameExW(hProcess, nullptr, path, MAX_PATH) > 0) {
        return std::wstring(path);
    }
    return L"";
}

// Coloque esta função na seção de Helpers ou Hooks
static LONG NTAPI VectoredAntiDebugHandler(PEXCEPTION_POINTERS ExceptionInfo) {
    if (!g_cfg.swallow_int3) return EXCEPTION_CONTINUE_SEARCH;
    if (ExceptionInfo && ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {
#ifdef _WIN64
        ExceptionInfo->ContextRecord->Rip++;
#else
        ExceptionInfo->ContextRecord->Eip++;
#endif
        LOG("[VectoredAntiDebugHandler]: consumiu INT3 em %p (modo agressivo)", ExceptionInfo->ExceptionRecord->ExceptionAddress);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD GetTIDFromHandle(HANDLE hThread)
{
    // GetThreadId funciona em WinVista+; não chama NtSetInformationThread
    typedef DWORD (WINAPI *PFN_GetThreadId)(HANDLE);
    static PFN_GetThreadId pGetThreadId =
        (PFN_GetThreadId)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadId");
    if (pGetThreadId) {
        DWORD tid = pGetThreadId(hThread);
        if (tid) return tid;
    }
    return 0;
}

static std::wstring ModuleFromAddress(const void* addr) {

    HMODULE mod = NULL;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)addr, &mod))
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(mod, path, MAX_PATH);
        const wchar_t* base = wcsrchr(path, L'\\');
        return base ? base + 1 : path;
    }
    return L"???.dll";
}


static inline bool IsExec(DWORD prot) {
    return (prot & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}
static inline bool IsReadable(DWORD prot) {
    return (prot & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)) != 0;
}

// ===== Util: módulo a partir de endereço =====
static std::wstring GetModuleFromAddress(void* addr) {
    HMODULE mod = NULL;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)addr, &mod)) {
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(mod, path, MAX_PATH);
        const wchar_t* base = wcsrchr(path, L'\\');
        return base ? (base + 1) : path;
    }
    return L"???.dll";
}

static void InitSelfModule(HMODULE hMod) {
    gSelfMod = hMod;
    MODULEINFO mi{};
    if (GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi))) {
        gSelfBase = (uintptr_t)mi.lpBaseOfDll;
        gSelfSize = (size_t)mi.SizeOfImage;
        LOG("[Self] base=%p size=%zu", (void*)gSelfBase, gSelfSize);
    }
}

// ===== Util: compor caminho de dump =====
static bool DumpMemoryRegion_Optimized(void* addr, SIZE_T size,
                                       DWORD protBefore, DWORD protAfter,
                                       const wchar_t* label,
                                       SIZE_T capBytes /*ex.: 16*MB*/)
{
    if (!size) return false;

    SYSTEM_INFO si; GetSystemInfo(&si);
    const SIZE_T page = si.dwPageSize;

    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)page - 1);
    uintptr_t end   = ((uintptr_t)addr + size + page - 1) & ~((uintptr_t)page - 1);
    SIZE_T    total = (SIZE_T)(end - start);

    if (capBytes && total > capBytes) {
        total = (capBytes + page - 1) & ~((SIZE_T)page - 1);
        end   = start + total;
    }

    std::wstring path = pedu::ComposeDumpPath((void*)start, total, protBefore, protAfter, label);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        LOG("[Dump] CreateFileW falhou (%lu)", GetLastError());
        return false;
    }

    // marcar como esparso (NTFS)
    DWORD br = 0;
    FILE_SET_SPARSE_BUFFER sparse = { TRUE };
    DeviceIoControl(h, FSCTL_SET_SPARSE, &sparse, sizeof(sparse), NULL, 0, &br, NULL);

    // pré-estende (buracos viram zeros lógicos)
    LARGE_INTEGER li; li.QuadPart = (LONGLONG)total;
    SetFilePointerEx(h, li, NULL, FILE_BEGIN);
    SetEndOfFile(h);
    li.QuadPart = 0;
    SetFilePointerEx(h, li, NULL, FILE_BEGIN);

    const SIZE_T CHUNK = 256 * 1024;
    std::vector<BYTE> buf(CHUNK);

    SIZE_T written = 0, gaps = 0;
    ULONGLONG t0 = GetTickCount64();

    uintptr_t cur = start;
    while (cur < end) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery((LPCVOID)cur, &mbi, sizeof(mbi))) break;

        uintptr_t rBase = (uintptr_t)mbi.BaseAddress;
        uintptr_t rEnd  = rBase + mbi.RegionSize;
        if (rBase >= end) break;

        uintptr_t blkStart = (cur > rBase) ? cur : rBase;
        uintptr_t blkEnd   = (rEnd < end) ? rEnd : end;
        SIZE_T    blkSize  = (SIZE_T)(blkEnd - blkStart);

        SIZE_T off = 0;
        while (off < blkSize) {
            SIZE_T toRead = blkSize - off;
            if (toRead > CHUNK) toRead = CHUNK;

            SIZE_T got = 0;
            BOOL ok = ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(blkStart + off), buf.data(), toRead, &got);

            if (ok && got) {
                DWORD wrote = 0;
                if (!WriteFile(h, buf.data(), (DWORD)got, &wrote, NULL) || wrote != got) {
                    LOG("[Dump] WriteFile falhou (%lu)", GetLastError());
                    CloseHandle(h);
                    return false;
                }
                written += got;

                if (got < toRead) {
                    LARGE_INTEGER skip; skip.QuadPart = (LONGLONG)(toRead - got);
                    SetFilePointerEx(h, skip, NULL, FILE_CURRENT);
                    gaps += (toRead - got);
                }
            } else {
                LARGE_INTEGER skip; skip.QuadPart = (LONGLONG)toRead;
                SetFilePointerEx(h, skip, NULL, FILE_CURRENT);
                gaps += toRead;
            }

            off += toRead;
        }

        cur = blkEnd;
    }

    CloseHandle(h);
    ULONGLONG dt = GetTickCount64() - t0;
    LOG("[Dump] %ws  total=%zu  dados=%zu  gaps=%zu  %llums",
        path.c_str(), (size_t)total, (size_t)written, (size_t)gaps, (unsigned long long)dt);
    return true;
}

static bool ieq(const std::wstring& a, const std::wstring& b){
    return ToLower(a) == ToLower(b);
}

// ===== Resolver PID a partir do HANDLE (tenta múltiplas rotas) =====
static DWORD GetPidFromHandleBestEffort(HANDLE hProc)
{
    // 1) GetProcessId (se disponível)
    typedef DWORD (WINAPI *PFN_GetProcessId)(HANDLE);
    static PFN_GetProcessId pGetProcessId =
        (PFN_GetProcessId)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessId");
    if (pGetProcessId) {
        DWORD pid = pGetProcessId(hProc);
        if (pid) return pid;
    }

    // 2) NtQueryInformationProcess(ProcessBasicInformation)
    static PFN_NtQueryInformationProcess pNtQIP =
        (PFN_NtQueryInformationProcess)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
    if (pNtQIP) {
        PROCESS_BASIC_INFORMATION_MIN pbi = {};
        ULONG ret = 0;
        if (pNtQIP(hProc, (PROCESSINFOCLASS)ProcessBasicInformation, &pbi, sizeof(pbi), &ret) >= 0) {
            return (DWORD)(uintptr_t)pbi.UniqueProcessId;
        }
    }

    // 3) Pseudohandle?
    if (hProc == GetCurrentProcess() || hProc == (HANDLE)(ULONG_PTR)-1)
        return GetCurrentProcessId();

    return 0;
}

// ===== Resolver nome do processo a partir do HANDLE (várias tentativas) =====
static bool QueryImageNameFromHandle(HANDLE hProc, std::wstring& outName)
{
    // 1) QueryFullProcessImageNameW
    typedef BOOL (WINAPI *PFN_QueryFullProcessImageNameW)(HANDLE,DWORD,LPWSTR,PDWORD);
    static PFN_QueryFullProcessImageNameW pQFPI =
        (PFN_QueryFullProcessImageNameW)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "QueryFullProcessImageNameW");
    if (pQFPI) {
        wchar_t buf[MAX_PATH]; DWORD c = MAX_PATH;
        if (pQFPI(hProc, 0, buf, &c)) {
            const wchar_t* base = wcsrchr(buf, L'\\');
            outName = base ? (base+1) : buf;
            return true;
        }
    }
    // 2) GetProcessImageFileNameW (PSAPI)
    wchar_t pbuf[MAX_PATH] = {0};
    if (GetProcessImageFileNameW(hProc, pbuf, MAX_PATH) > 0) {
        const wchar_t* base = wcsrchr(pbuf, L'\\');
        outName = base ? (base+1) : pbuf;
        return true;
    }
    // 3) NtQueryInformationProcess(ProcessImageFileNameWin32 / ProcessImageFileName)
    static PFN_NtQueryInformationProcess pNtQIP =
        (PFN_NtQueryInformationProcess)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
    if (pNtQIP) {
        // Try Win32 first (path em formato Win32)
        struct { USHORT Length; USHORT MaximumLength; PWSTR Buffer; } name = {0};
        ULONG ret = 0;
        if (pNtQIP(hProc, (PROCESSINFOCLASS)ProcessImageFileNameWin32, &name, sizeof(name), &ret) >= 0 && name.Buffer) {
            std::wstring full(name.Buffer, name.Length/sizeof(WCHAR));
            const wchar_t* base = wcsrchr(full.c_str(), L'\\');
            outName = base ? (base+1) : full;
            return true;
        }
        // Fallback: ProcessImageFileName (formato NT)
        name = {0};
        if (pNtQIP(hProc, (PROCESSINFOCLASS)ProcessImageFileName, &name, sizeof(name), &ret) >= 0 && name.Buffer) {
            std::wstring full(name.Buffer, name.Length/sizeof(WCHAR));
            const wchar_t* base = wcsrchr(full.c_str(), L'\\');
            outName = base ? (base+1) : full;
            return true;
        }
    }
    return false;
}

// ===== Cache por PID (rápido, thread-safe) =====

static std::wstring GetProcessNameFromHandle_Cached(HANDLE hProc)
{
    EnsureProcNameCs();
    DWORD pid = GetPidFromHandleBestEffort(hProc);
    if (!pid) return L"(unknown)";

    const ULONGLONG now = GetTickCount64();
    const ULONGLONG TTL = 5000; // 5s

    {
        EnterCriticalSection(&gProcNameCs);
        auto it = gProcNameCache.find(pid);
        if (it != gProcNameCache.end() && (now - it->second.lastSeenMs) < TTL) {
            std::wstring n = it->second.name;
            LeaveCriticalSection(&gProcNameCs);
            return n;
        }
        LeaveCriticalSection(&gProcNameCs);
    }

    std::wstring name;
    if (!QueryImageNameFromHandle(hProc, name)) {
        // Tenta abrir por PID se não conseguir usar o handle passado
        HANDLE dup = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (dup) {
            QueryImageNameFromHandle(dup, name);
            CloseHandle(dup);
        }
        if (name.empty()) name = L"(pid:" + std::to_wstring(pid) + L")";
    }

    EnterCriticalSection(&gProcNameCs);
    gProcNameCache[pid] = { name, now };
    // Simples controle de tamanho
    if (gProcNameCache.size() > 256) gProcNameCache.clear();
    LeaveCriticalSection(&gProcNameCs);

    return name;
}

// ===== Módulo a partir do endereço do chamador (melhora contexto) =====
static std::wstring GetModuleFromAddressW(void* addr) {
    HMODULE mod = NULL;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)addr, &mod))
    {
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(mod, path, MAX_PATH);
        const wchar_t* base = wcsrchr(path, L'\\');
        return base ? (base + 1) : path;
    }
    return L"???.dll";
}

// ===== Heurísticas simples =====
static bool IsBrowser(const std::wstring& nameLower) {
    static const wchar_t* browsers[] = {
        L"chrome.exe", L"msedge.exe", L"brave.exe", L"opera.exe", L"opera_browser.exe",
        L"firefox.exe", L"vivaldi.exe"
    };
    for (auto s : browsers) if (nameLower == s) return true;
    return false;
}


// ===== (Opcional) Anotar proteção da região remota =====
static DWORD ProtOfRemote(HANDLE hProc, LPCVOID addr) {
    MEMORY_BASIC_INFORMATION mbi = {};
    SIZE_T q = VirtualQueryEx(hProc, addr, &mbi, sizeof(mbi));
    return q ? mbi.Protect : 0;
}

#ifndef UEF_POLICY
#define UEF_POLICY 1   // 0 = BLOCK, 1 = CHAIN
#endif

static thread_local bool g_inUEF = false;
static std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> g_currMalwareUEF{ nullptr };
static std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> g_currTrampoline{ nullptr };

// helper para “chamar o original do UEF” (KernelBase > Kernel32)
static inline pSetUnhandledExceptionFilter CallOrig_UEF() {
    if (originalSetUnhandledExceptionFilterKB) return originalSetUnhandledExceptionFilterKB;
    return originalSetUnhandledExceptionFilter;
}


// Nosso detour do handler do malware (modo CHAIN)
static LONG WINAPI HookedMalwareUEF(EXCEPTION_POINTERS* p)
{
    auto malwareUEF = g_currMalwareUEF.load();
    auto origCall   = g_currTrampoline.load();

    // Log contextual da exceção
    PEXCEPTION_RECORD er = p ? p->ExceptionRecord : nullptr;
    void* faultAddr = er ? er->ExceptionAddress : nullptr;
    DWORD code      = er ? er->ExceptionCode    : 0;

    LOG("[UEF/CHAIN] excecao code=0x%08X at=%p  callerUEF=%p (%ls)",code, faultAddr, malwareUEF, ModuleFromAddress(FADDR(malwareUEF)).c_str());


    // Aqui você pode aplicar políticas (e.g., suprimir exceções anti-debug):
    // - retornar EXCEPTION_CONTINUE_SEARCH para ignorar o handler do malware
    // - ou chamar o handler original (trampolim) para manter comportamento

    if (origCall) {
        return origCall(p); // chama o UEF original do malware (encadeado)
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// =========================
//   Hooks
// =========================
// ========================= user32.dll =========================
// helpers usados por hooks de user32
struct EnumWindowsCallbackData {
    WNDENUMPROC originalCallback;
    LPARAM originalLParam;
};

static BOOL CALLBACK FilteringEnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<EnumWindowsCallbackData*>(lParam);

    wchar_t title[256] = L"", cls[256] = L"";
    GetWindowTextW(hwnd, title, 256);
    GetClassNameW(hwnd, cls, 256);

    std::wstring t = ToLower(title), c = ToLower(cls);
    for (const auto& blocked : g_blockedWindowNames) {
        if (ci_contains(t, blocked) || ci_contains(c, blocked)) {
            LOG("EnumWindows: ocultando janela suspeita: '%ls' (classe '%ls')", title, cls);
            return TRUE; // pula esta janela, segue enumeração
        }
    }
    return data->originalCallback(hwnd, data->originalLParam);
}

// HOOKS (user32.dll)
static BOOL WINAPI HookedEnumWindows(WNDENUMPROC lpEnumFunc, LPARAM lParam) {
    LOG("EnumWindows() chamado; aplicando filtro case-insensitive");
    EnumWindowsCallbackData d{ lpEnumFunc, lParam };
    return originalEnumWindows(FilteringEnumWindowsProc, reinterpret_cast<LPARAM>(&d));
}

static HWND WINAPI HookedFindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) {
    std::wstring t = ToLower(lpWindowName ? lpWindowName : L"");
    std::wstring c = ToLower(lpClassName ? lpClassName : L"");
    for (const auto& blocked : g_blockedWindowNames) {
        if ((!t.empty() && ci_contains(t, blocked)) || (!c.empty() && ci_contains(c, blocked))) {
            LOG("FindWindowW BLOQUEADO (titulo='%ls', classe='%ls')", lpWindowName ? lpWindowName : L"", lpClassName ? lpClassName : L"");
            SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
            return NULL;
        }
    }
    return originalFindWindowW(lpClassName, lpWindowName);
}



// ========================= kernel32.dll =======================
// HOOKS (kernel32.dll)
static HANDLE WINAPI HookedCreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID) {
    LOG("[CreateToolhelp32Snapshot](dwFlags=0x%X, th32ProcessID=%lu)", dwFlags, th32ProcessID);
    return originalCreateToolhelp32Snapshot(dwFlags, th32ProcessID);
}

static BOOL WINAPI HookedProcess32FirstW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) {
    BOOL result = originalProcess32FirstW(hSnapshot, lppe);
    while (result && IsSecurityProcessName(lppe->szExeFile)) {
        LOG("Process32FirstW: Ocultando processo suspeito: %ls (PID: %lu)", lppe->szExeFile, lppe->th32ProcessID);
        result = originalProcess32NextW(hSnapshot, lppe);
    }
    return result;
}

static BOOL WINAPI HookedProcess32NextW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) {
    BOOL result = originalProcess32NextW(hSnapshot, lppe);
    while (result && IsSecurityProcessName(lppe->szExeFile)) {
        LOG("Process32NextW: Ocultando processo suspeito: %ls (PID: %lu)", lppe->szExeFile, lppe->th32ProcessID);
        result = originalProcess32NextW(hSnapshot, lppe);
    }
    return result;
}

static BOOL WINAPI HookedIsDebuggerPresent()
{
    void* returnAddress = _ReturnAddress();
    DWORD threadId = GetCurrentThreadId();

    LOG("[IsDebuggerPresent] :");
    LOG("  -> Endereço da chamada (Caller EIP/RIP): 0x%p", returnAddress);
    LOG("  -> ID da Thread: %lu", threadId);

    // Stack completo a partir do call-site
    WalkStackFromReturnAddress(returnAddress, 64);

    LOG("  -> Ação: Retornando FALSE.");
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
        if (!name.empty() && IsSecurityProcessName(name)) {
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
    //EXEMPLO:: OpenProcess(pid=..., access=0x...)
    //LOG("OpenProcess(pid=%lu, access=0x%X)", pid, access);   
    return originalOpenProcess(access, inherit, pid);
}
static thread_local bool g_inRPM = false;
static BOOL WINAPI HookedReadProcessMemory(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    LPVOID  lpBuffer,
    SIZE_T  nSize,
    SIZE_T* lpNumberOfBytesRead)
{
    // evita loops caso seu logger toque RPM/VirtualQueryEx
    if (g_inRPM) {
        return CallOrig_ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
    }
    g_inRPM = true;

    // — contexto do chamador —
    void* callerAddress = GET_RETURN_ADDRESS();
    std::wstring callerMod = GetModuleFromAddressW(callerAddress);

    // — alvo —
    const BOOL isSelf = (hProcess == GetCurrentProcess()) || (hProcess == (HANDLE)(ULONG_PTR)-1);
    std::wstring procName = isSelf ? L"(self)" : GetProcessNameFromHandle_Cached(hProcess);

    // — heurísticas —
    if (!isSelf) {
        std::wstring procLower = ToLower(procName);
        DWORD prot = ProtOfRemote(hProcess, lpBaseAddress);

        if (IsSecurityProcessName(procName)) {
            // RISCO ALTO: log 100% das vezes
        
            LOG("[ReadProcessMemory] {\"event\":\"rpm\",\"risk\":\"high\",\"target\":\"%ls\",\"addr\":\"%p\",\"size\":%llu,\"prot\":\"0x%08X\",\"caller\":\"%p\",\"caller_mod\":\"%ls\"}",
                      procName.c_str(), lpBaseAddress, (unsigned long long)nSize, prot, callerAddress, callerMod.c_str());
            LOG("[ReadProcessMemory] !!! ALTO RISCO: leitura em processo sensível (%ls)", procName.c_str());
            LOG("  -> End.: %p  Tam.: %llu  Prot: 0x%08X", lpBaseAddress, (unsigned long long)nSize, prot);
            LOG("  -> Caller: %p (%ls)", callerAddress, callerMod.c_str());
      
            // (opcional) stack completo
            WalkStackFromReturnAddress(callerAddress, 48);
        }
        else {
            // RISCO BAIXO: amostrar por janela + tamanho
            static ULONGLONG lastTick = 0; static ULONG cnt = 0;
            ULONGLONG now = GetTickCount64();
            if (now - lastTick >= 1000) { lastTick = now; cnt = 0; }
            ++cnt;

            if (nSize >= 0x2000 || (cnt % 128) == 0) {
            
                LOG("[ReadProcessMemory] {\"event\":\"rpm\",\"risk\":\"low\",\"target\":\"%ls\",\"addr\":\"%p\",\"size\":%llu,\"prot\":\"0x%08X\",\"caller\":\"%p\",\"caller_mod\":\"%ls\"}",
                    procName.c_str(), lpBaseAddress, (unsigned long long)nSize, prot, callerAddress, callerMod.c_str());
            
                LOG("[ReadProcessMemory] amostra: alvo=%ls end=%p size=%llu prot=0x%08X caller=%p (%ls)",
                    procName.c_str(), lpBaseAddress, (unsigned long long)nSize, prot, callerAddress, callerMod.c_str());
            
            }
        }
    }

    // — chamada real —
    BOOL ok = CallOrig_ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
    SIZE_T got = lpNumberOfBytesRead ? *lpNumberOfBytesRead : 0;


    LOG("[ReadProcessMemory] {\"event\":\"rpm_ret\",\"status\":%d,\"bytes\":%llu}", ok ? 1 : 0, (unsigned long long)got);

    if (!ok) {
        LOG("[ReadProcessMemory] falha (GLE=%lu) bytes=%llu", GetLastError(), (unsigned long long)got);
    }


    g_inRPM = false;
    return ok;
}

static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER f) {
    if (g_inUEF) {
        auto callUEF = CallOrig_UEF();
        return callUEF ? callUEF(f) : nullptr;
    }
    g_inUEF = true;

    void* ret = GET_RETURN_ADDRESS();
    auto callerMod  = GetModuleFromAddressW(ret);
    auto handlerMod = GetModuleFromAddressW((void*)f);

#if UEF_POLICY == 0
    // ---------- BLOQUEAR ----------
    auto callUEF = CallOrig_UEF();
    LPTOP_LEVEL_EXCEPTION_FILTER prev = nullptr;

    if (callUEF) {
        // truque para obter o “prev” e manter o estado
        prev = callUEF(nullptr);
        callUEF(prev);
    }
    if (!IsFromSelf(FADDR(f))) {
        LOG("[UEF/BLOCK] bloqueado SetUnhandledExceptionFilter(f=%p %ls) caller=%ls  prev=%p %ls",
            f, handlerMod.c_str(), callerMod.c_str(), prev, ModuleFromAddress(FADDR(prev)).c_str());
    }
    g_inUEF = false;
    return prev;

#else
    // ---------- CHAIN ----------
    if (!f || IsFromSelf(FADDR(f))) { // nada para fazer
        auto callUEF2 = CallOrig_UEF();
        auto prev = callUEF2 ? callUEF2(f) : nullptr;
        LOG("[UEF/CHAIN] pass-through f=%p (%ls) caller=%ls prev=%p",
            f, GetModuleFromAddressW((void*)f).c_str(), callerMod.c_str(), prev);
        g_inUEF = false;
        return prev;
    }

    LPTOP_LEVEL_EXCEPTION_FILTER tmpOrig = nullptr;
    if (MH_CreateHook((LPVOID)f, (LPVOID)HookedMalwareUEF, (LPVOID*)&tmpOrig) == MH_OK) {
        MH_EnableHook((LPVOID)f);
        g_currMalwareUEF.store(f);
        g_currTrampoline.store(tmpOrig);
    }

    auto callUEF3 = CallOrig_UEF();
    auto prev = callUEF3 ? callUEF3(f) : nullptr;

    LOG("[UEF/CHAIN] instalado f=%p (%ls) caller=%ls  prev=%p (%ls) tramp=%p",
        f,
        GetModuleFromAddressW((void*)f).c_str(),
        callerMod.c_str(),
        prev,
        ModuleFromAddress(FADDR(prev)).c_str(),
        FADDR(g_currTrampoline.load()));

    g_inUEF = false;
    return prev;
#endif
}


static thread_local bool g_inVP = false;
// limites pragmáticos de dump
static const SIZE_T MAX_BEFORE = 16 * 1024 * 1024; // 16MB antes
static const SIZE_T MAX_AFTER  = 64 * 1024 * 1024; // 64MB depois

static BOOL WINAPI HookedVirtualProtect(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flNewProtect,
    PDWORD lpflOldProtect)
{
    if (g_inVP) {
        return CallOrig_VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
    }
    g_inVP = true;

    // 1) estado atual
    MEMORY_BASIC_INFORMATION mbi = {};
    VirtualQuery(lpAddress, &mbi, sizeof(mbi));
    DWORD protectionBefore = mbi.Protect;

    void* callerAddress = GET_RETURN_ADDRESS();
    std::wstring targetModule = GetModuleFromAddress(lpAddress);
    std::wstring callerModule = GetModuleFromAddress(callerAddress);

    // 2) detecção NX -> X
    bool wasExecutable     = IsExec(protectionBefore);
    bool becomesExecutable = IsExec(flNewProtect);

    if (!wasExecutable && becomesExecutable) {
        // ignore eventos gerados por esta própria DLL (caller ou região)
        if (!IsFromSelf(callerAddress) && !IsFromSelf(lpAddress)) {
            LOG("[VirtualProtect] !!! ALERTA: RW/NX -> EXEC");
            LOG("  -> Target: %p (%ls), Size: %zu", lpAddress, targetModule.c_str(), (size_t)dwSize);
            LOG("  -> Protect: 0x%08X -> 0x%08X", protectionBefore, flNewProtect);
            LOG("  -> Caller : %p (%ls)", callerAddress, callerModule.c_str());

            // snapshot ANTES (cap para não travar)
            DumpMemoryRegion_Optimized(lpAddress, dwSize, protectionBefore, flNewProtect, L"before_exec", MAX_BEFORE);
        }
    }

    // 3) chamar original
    DWORD oldProtInternal = 0;
    BOOL ok = CallOrig_VirtualProtect(lpAddress, dwSize, flNewProtect, &oldProtInternal);

    if (!ok) {
        LOG("[VirtualProtect] ERRO GetLastError=%lu", GetLastError());
    }

    // 3b) rebuild/scan + snapshot DEPOIS somente se troca ocorreu e sucesso
    if (ok && !wasExecutable && becomesExecutable
        && !IsFromSelf(callerAddress) && !IsFromSelf(lpAddress))
    {
        MEMORY_BASIC_INFORMATION mbi2{};
        VirtualQuery(lpAddress, &mbi2, sizeof(mbi2));

        std::wstring outHash;
        if (mbi2.Type == MEM_IMAGE) {
            auto out = pedu::ComposeDumpPathPE(mbi2.AllocationBase, L"rebuilt_after_exec.exe");
            if (pedu::DumpPeMappedImageToFile(mbi2.AllocationBase, out, &outHash)) {
                LOG("[PEdump] Rebuilt MEM_IMAGE -> %ls  sha256=%ls", out.c_str(), outHash.c_str());
            }
        } else {
            SIZE_T scanCap = std::min<SIZE_T>(dwSize, 8 * 1024 * 1024); // varrer até 8MB
            auto out = pedu::ComposeDumpPathPE(lpAddress, L"scanned_after_exec.exe");
            void* found = nullptr;
            if (pedu::ScanAndDumpPeInRange(lpAddress, scanCap, out, &found, &outHash)) {
                LOG("[PEscan] Found PE at %p -> %ls  sha256=%ls", found, out.c_str(), outHash.c_str());
            }
        }

        // snapshot DEPOIS
        DumpMemoryRegion_Optimized(lpAddress, dwSize, protectionBefore, flNewProtect, L"after_exec", MAX_AFTER);
    }

    // 4) contrato da API
    if (lpflOldProtect) *lpflOldProtect = oldProtInternal;

    g_inVP = false;
    return ok;
}


static BOOL WINAPI HookedVirtualProtectEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect) {
    if (hProcess != GetCurrentProcess() && (flNewProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        DWORD targetPid = GetProcessId(hProcess);
        std::wstring modulePath = GetProcessModulePath(hProcess);
        LOG("VirtualProtectEx EXEC alvo PID=%lu mod='%ls' base=%p size=%llu newProt=0x%X",
            targetPid, modulePath.c_str(), lpAddress, (unsigned long long)dwSize, flNewProtect);
    }

    if (!g_cfg.force_write_on_prot) {
        // pass-through (padrão)
        return originalVirtualProtectEx(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
    }

    // modo agressivo: força escrita
    DWORD modifiable = flNewProtect;
    if (!(flNewProtect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_WRITECOPY))) {
        if (flNewProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ))       modifiable = PAGE_EXECUTE_READWRITE;
        else if (flNewProtect & PAGE_READONLY)                       modifiable = PAGE_READWRITE;
        else                                                         modifiable |= PAGE_WRITECOPY;
        LOG("VirtualProtectEx (AGGR): forçando escrita 0x%X->0x%X", flNewProtect, modifiable);
    }
    return originalVirtualProtectEx(hProcess, lpAddress, dwSize, modifiable, lpflOldProtect);
}

static SIZE_T WINAPI HookedVirtualQuery(LPCVOID addr, PMEMORY_BASIC_INFORMATION mbi, SIZE_T len) {
    static unsigned q = 0;
    SIZE_T r = originalVirtualQuery(addr, mbi, len);
    if (r && mbi && (mbi->Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        if ((++q & 0x3F) == 0)
            LOG("VirtualQuery EXEC region base=%p size=%llu", mbi->BaseAddress, (unsigned long long)mbi->RegionSize);
    }
    return r;
}

static HANDLE WINAPI HookedCreateRemoteThread(HANDLE hProcess, LPSECURITY_ATTRIBUTES a, SIZE_T st, LPTHREAD_START_ROUTINE start, LPVOID param, DWORD flags, LPDWORD tid) {
    LOG("CreateRemoteThread alvo=%p start=%p flags=0x%X", hProcess, start, flags);
    return originalCreateRemoteThread(hProcess, a, st, start, param, flags, tid);
}

static DWORD WINAPI HookedResumeThread(HANDLE hThread) {
    LOG("ResumeThread(thread=%p)", hThread);
    return originalResumeThread(hThread);
}

static VOID WINAPI HookedExitProcess(UINT code) {
    LOG("ExitProcess(code=%u)", code);

    __debugbreak(); 

    originalExitProcess(code);
}

static BOOL WINAPI HookedTerminateProcess(HANDLE hProc, UINT code) {
    if (hProc == GetCurrentProcess()) {
        LOG("!!! BLOQUEADO: Tentativa de [TerminateProcess] no próprio processo (code=%u)", code);
        return TRUE; // simula sucesso
    }
    LOG("[TerminateProcess](proc=%p, code=%u)", hProc, code);
    return originalTerminateProcess(hProc, code);
}

static LPVOID WINAPI HookedVirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
    LPVOID allocatedAddress = originalVirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
    if (hProcess != GetCurrentProcess() && (flProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        DWORD targetPid = GetProcessId(hProcess);
        std::wstring modulePath = GetProcessModulePath(hProcess);
        LOG("!!! LINHA VERMELHA: VirtualAllocEx com EXECUTE em processo alvo. PID: %lu, Módulo: '%ls', Base: %p, Tamanho: %llu",
            targetPid, modulePath.c_str(), allocatedAddress, (unsigned long long)dwSize);
    } else {
        LOG("VirtualAllocEx() chamado. Processo: %p, Tamanho: %llu, Tipo: 0x%X, Proteção: 0x%X", hProcess, (unsigned long long)dwSize, flAllocationType, flProtect);
    }
    return allocatedAddress;
}

static BOOL WINAPI HookedWriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten) {
    LOG("WriteProcessMemory() chamado. Processo: %p, Endereço destino: %p, Tamanho: %llu", hProcess, lpBaseAddress, (unsigned long long)nSize);
    return originalWriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

static HANDLE WINAPI HookedCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    if (dwCreationFlags & CREATE_SUSPENDED) LOG("!!! ALERTA: CreateThread com CREATE_SUSPENDED (0x4). Start: %p", lpStartAddress);
    else                                    LOG("CreateThread chamada. Start: %p", lpStartAddress);
    HANDLE hThread = originalCreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    if (hThread && (dwCreationFlags & CREATE_SUSPENDED)) {
        LOG("!!! THREAD SUSPEITA CRIADA! Handle: %p, ID: %lu", hThread, (lpThreadId ? *lpThreadId : 0));
    }
    return hThread;
}

static BOOL WINAPI HookedDebugActiveProcess(DWORD pid) {
    if (pid == GetCurrentProcessId()) {
        LOG("DebugActiveProcess(self) BLOQUEADO -> FALSE/ACCESS_DENIED");
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return originalDebugActiveProcess(pid);
}

static VOID WINAPI HookedOutputDebugStringW(LPCWSTR lpOutputString) {
    if (lpOutputString) LOG("[OutputDebugStringW] \"%ls\"", lpOutputString);
    if (g_cfg.swallow_ods) return; // modo agressivo: engole a mensagem
    return originalOutputDebugStringW(lpOutputString); // padrão: deixa passar
}

// ========================= advapi32.dll =======================
// HOOKS (advapi32.dll)
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



// ========================== ntdll.dll =========================
// HOOKS (ntdll.dll)
static NTSTATUS NTAPI HookedNtOpenProcess(PHANDLE ph, ACCESS_MASK acc, POBJECT_ATTRIBUTES oa, PCLIENT_ID cid) {
    DWORD pid = (cid && cid->UniqueProcess) ? (DWORD)(ULONG_PTR)cid->UniqueProcess : 0;
    if (pid) {
        std::wstring name = GetExeNameByPid(pid);
        if (!name.empty() && IsSecurityProcessName(name)) {
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

static NTSTATUS NTAPI HookedNtCreateThreadEx(PHANDLE th, ACCESS_MASK da, POBJECT_ATTRIBUTES oa,
                                             HANDLE proc, LPTHREAD_START_ROUTINE start, PVOID arg,
                                             ULONG cf, ULONG_PTR zb, SIZE_T ss, SIZE_T mss, PVOID attrs)
{
    DWORD pid = 0;
    if (proc && proc != INVALID_HANDLE_VALUE) pid = GetProcessId(proc);
    if (pid && pid != GetCurrentProcessId()) {
        LOG("NtCreateThreadEx REMOTA no pid=%lu start=%p", pid, (PVOID)start);
    }
    return originalNtCreateThreadEx(th, da, oa, proc,
                                    reinterpret_cast<PVOID>(start),
                                    arg, cf, zb, ss, mss, attrs);
}

static NTSTATUS NTAPI HookedNtTerminateProcess(HANDLE hProc, NTSTATUS status) {
    if (g_cfg.block_self_kill && hProc == GetCurrentProcess()) {
        LOG("BLOQUEADO: NtTerminateProcess(self, 0x%08X)", (unsigned)status);
        return STATUS_SUCCESS;
    }
    LOG("NtTerminateProcess(proc=%p, status=0x%08X)", hProc, (unsigned)status);
    return originalNtTerminateProcess(hProc, status);
}

// C++11+ (compilando com -std=gnu++17)
static thread_local int g_inHook_NSI = 0;

// RAII para garantir decremento mesmo em early return
struct RecGuardNSI {
    RecGuardNSI()  { ++g_inHook_NSI; }
    ~RecGuardNSI() { --g_inHook_NSI; }
    bool reentered() const { return g_inHook_NSI > 1; }
};


static NTSTATUS NTAPI HookedNtSetInformationThread(
    HANDLE threadHandle,
    THREADINFOCLASS threadInformationClass,
    PVOID threadInformation,
    ULONG threadInformationLength
) {
    // Evita recursão se algo dentro do hook chamar novamente esta API
    if (g_inHook_NSI++) { 
        // fallback: chama o original imediatamente
        NTSTATUS st = originalNtSetInformationThread
            ? originalNtSetInformationThread(threadHandle, threadInformationClass, threadInformation, threadInformationLength)
            : (NTSTATUS)0xC0000002; // STATUS_NOT_IMPLEMENTED
        g_inHook_NSI--;
        return st;
    }

    void* returnAddress = _ReturnAddress();
    DWORD callerTID = GetCurrentThreadId();

    // Metadados da thread alvo
    DWORD targetTID = 0;
    if (threadHandle == GetCurrentThread()) {
        targetTID = callerTID;
    } else {
        targetTID = GetTIDFromHandle(threadHandle);
    }

    auto callerMod = GetModuleFromAddressW(returnAddress);

    if (threadInformationClass == ThreadHideFromDebugger /* 0x11 */)
    {
        LOG("[ANTI-DEBUG] NtSetInformationThread(ThreadHideFromDebugger)");
        LOG("  Caller RIP: %p  (mod: %ls)", returnAddress, callerMod.c_str());
        LOG("  Exec TID: %lu  Target TID: %lu  Handle: %p", callerTID, targetTID, threadHandle);
        LOG("  InfoBuffer: %p  Length: %lu", threadInformation, (unsigned long)threadInformationLength);

        // Stack completo a partir do call-site (se tiver salker.h)
        WalkStackFromReturnAddress(returnAddress, 64);

    #if AB_BYPASS_THREADHIDE
        LOG("  Ação: BYPASS habilitado -> retornando STATUS_SUCCESS (spoof).");
        g_inHook_NSI--;
        return STATUS_SUCCESS;
    #else
        // Observação fiel (recomendado p/ TCC): chama o original e loga resultado
        NTSTATUS st = originalNtSetInformationThread
            ? originalNtSetInformationThread(threadHandle, threadInformationClass, threadInformation, threadInformationLength)
            : (NTSTATUS)0xC0000002; // STATUS_NOT_IMPLEMENTED

        LOG("  Retorno real: 0x%08X (%s)", (unsigned)st, NT_SUCCESS(st) ? "SUCCESS" : "FAIL");
        g_inHook_NSI--;
        return st;
    #endif
    }

    // Outras classes -> encaminha para o original
    NTSTATUS st = originalNtSetInformationThread
        ? originalNtSetInformationThread(threadHandle, threadInformationClass, threadInformation, threadInformationLength)
        : (NTSTATUS)0xC0000002; // STATUS_NOT_IMPLEMENTED

    g_inHook_NSI--;
    return st;
}

// HOOK seguro e furtivo (versão final recomendada)
static NTSTATUS NTAPI HookedNtQueryInformationProcess(
    HANDLE proc, 
    PROCESSINFOCLASS pic, 
    PVOID info, 
    ULONG infolen, 
    PULONG retlen
) {
    // Sempre chama a função original primeiro para obter dados realistas
    NTSTATUS status = originalNtQueryInformationProcess(proc, pic, info, infolen, retlen);

    // Só modifica se a chamada original foi bem-sucedida
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Modifica cirurgicamente os resultados para enganar as checagens anti-debug
    switch ((int)pic) {
        case 0: { // ProcessBasicInformation
            if (info && infolen >= sizeof(PROCESS_BASIC_INFORMATION)) {
                PROCESS_BASIC_INFORMATION* pbi = (PROCESS_BASIC_INFORMATION*)info;
                if (pbi->PebBaseAddress) {
                    // Acesso seguro ao PEB para compatibilidade com MinGW
                    pbi->PebBaseAddress->BeingDebugged = 0;
                }
            }
            break;
        }

        case 7: { // ProcessDebugPort
            if (info && infolen >= sizeof(HANDLE)) {
                *(PHANDLE)info = (HANDLE)0; // Retorna 0 para indicar que não há porta de debug
            }
            break;
        }

        case 30: { // ProcessDebugObjectHandle
            return (NTSTATUS)0xC0000353; // Retorna STATUS_PORT_NOT_SET, uma resposta mais sutil
        }

        case 31: { // ProcessDebugFlags
            if (info && infolen >= sizeof(ULONG)) {
                *(PULONG)info = 1; // Retorna NoDebugInherit
            }
            break;
        }
    }

    return status;
}

static NTSTATUS NTAPI HookedNtQueryVirtualMemory(HANDLE proc, PVOID base, ULONG mic, PVOID mem, SIZE_T len, PSIZE_T out) {
    return originalNtQueryVirtualMemory(proc, base, mic, mem, len, out);
}

static NTSTATUS NTAPI HookedNtReadVirtualMemory(
    HANDLE ProcessHandle,
    PVOID  BaseAddress,
    PVOID  Buffer,
    SIZE_T NumberOfBytesToRead,
    PSIZE_T NumberOfBytesRead)
{
    static thread_local bool g_inHook = false;
    if (g_inHook) {
        return originalNtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesRead);
    }
    g_inHook = true;

    // Filtro de ruído (janela 1s)
    static ULONGLONG lastTick = 0; static ULONG readsInWindow = 0;
    ULONGLONG now = GetTickCount64();
    if (now - lastTick >= 1000) { lastTick = now; readsInWindow = 0; }
    ++readsInWindow;

    const BOOL isSelf = (ProcessHandle == GetCurrentProcess()) || (ProcessHandle == (HANDLE)(ULONG_PTR)-1);

    void* callerAddress = GET_RETURN_ADDRESS();
    std::wstring callerMod = GetModuleFromAddressW(callerAddress);

    if (!isSelf) {
        std::wstring procName = GetProcessNameFromHandle_Cached(ProcessHandle);

        // Heurística: alvo crítico
        if (IsSecurityProcessName(procName)) {

            LOG("[NtReadVirtualMemory]  {\"event\":\"ntreadvm\",\"risk\":\"high\",\"why\":\"security_process\",\"target\":\"%ls\",\"addr\":\"%p\",\"size\":%llu,\"caller\":\"%p\",\"caller_mod\":\"%ls\"}",
                procName.c_str(), BaseAddress, (unsigned long long)NumberOfBytesToRead, callerAddress, callerMod.c_str());

            LOG("[NtReadVirtualMemory] Alvo de segurança (%ls) — possível evasão/ataque a defensivo.", procName.c_str());
            LOG("  -> Endereço: %p  Tamanho: %llu  Caller: %p (%ls)", BaseAddress, (unsigned long long)NumberOfBytesToRead, callerAddress, callerMod.c_str());

        }
        else {
            // Amostragem para não inundar
            if (NumberOfBytesToRead >= 0x1000 || (readsInWindow % 64) == 0) {
                DWORD prot = ProtOfRemote(ProcessHandle, BaseAddress);

                LOG("[NtReadVirtualMemory]  {\"event\":\"ntreadvm\",\"risk\":\"low\",\"target\":\"%ls\",\"addr\":\"%p\",\"size\":%llu,\"prot\":\"0x%08X\",\"caller\":\"%p\",\"caller_mod\":\"%ls\"}",
                    procName.c_str(), BaseAddress, (unsigned long long)NumberOfBytesToRead, prot, callerAddress, callerMod.c_str());

                LOG("[NtReadVirtualMemory] Leitura interprocesso: alvo=%ls end=%p size=%llu prot=0x%08X caller=%p (%ls)",
                    procName.c_str(), BaseAddress, (unsigned long long)NumberOfBytesToRead, prot, callerAddress, callerMod.c_str());

            }
        }

        // (Opcional) Stack walk para contexto completo (requer salker.h)
        WalkStackFromReturnAddress(callerAddress, 48);
    }

    NTSTATUS st = originalNtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesRead);

    LOG("[NtReadVirtualMemory]  {\"event\":\"ntreadvm_ret\",\"status\":\"0x%08X\",\"bytes_read\":%llu}", (unsigned)st, (unsigned long long)(NumberOfBytesRead ? *NumberOfBytesRead : 0));

    if (st < 0) {
        LOG("[NtReadVirtualMemory] Retorno=0x%08X  (falha) bytes_read=%llu", (unsigned)st, (unsigned long long)(NumberOfBytesRead ? *NumberOfBytesRead : 0));
    }


    g_inHook = false;
    return st;
}


static NTSTATUS NTAPI HookedNtQueueApcThread(HANDLE ThreadHandle, PVOID ApcRoutine, PVOID ApcArgument1, PVOID ApcArgument2, PVOID ApcArgument3) {
    LOG("!!! ALERTA DE INJEÇÃO APC: NtQueueApcThread chamado para a thread handle %p. Rotina: %p", ThreadHandle, ApcRoutine);
    return originalNtQueueApcThread(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
}

static NTSTATUS NTAPI HookedNtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten) {
    LOG("NtWriteVirtualMemory() chamado. Processo: %p, Endereço: %p, Tamanho: %llu", ProcessHandle, BaseAddress, (unsigned long long)NumberOfBytesToWrite);
    return originalNtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
}

static NTSTATUS NTAPI HookedNtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect) {
    SIZE_T size = (RegionSize ? *RegionSize : 0);
    LOG("NtAllocateVirtualMemory() chamado. Processo: %p, Tamanho: %llu, Tipo: 0x%X, Proteção: 0x%X", ProcessHandle, (unsigned long long)size, AllocationType, Protect);
    return originalNtAllocateVirtualMemory(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

static NTSTATUS NTAPI HookedNtProtectVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect) {
    if (!g_cfg.force_write_on_prot) {
        return originalNtProtectVirtualMemory(ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
    }

    ULONG modifiable = NewProtect;
    if (!(NewProtect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_WRITECOPY))) {
        if (NewProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ))       modifiable = PAGE_EXECUTE_READWRITE;
        else if (NewProtect & PAGE_READONLY)                       modifiable = PAGE_READWRITE;
        else                                                       modifiable |= PAGE_WRITECOPY;
        LOG("NtProtectVirtualMemory (AGGR): 0x%X->0x%X", NewProtect, modifiable);
    }
    return originalNtProtectVirtualMemory(ProcessHandle, BaseAddress, RegionSize, modifiable, OldProtect);
}


static NTSTATUS NTAPI HookedNtQuerySystemInformation(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
) {
    // Ignora se não for SystemProcessInformation
    if (SystemInformationClass != 5 /* SystemProcessInformation */) {
        return originalNtQuerySystemInformation(
            SystemInformationClass,
            SystemInformation,
            SystemInformationLength,
            ReturnLength
        );
    }

    NTSTATUS status = originalNtQuerySystemInformation(
        SystemInformationClass,
        SystemInformation,
        SystemInformationLength,
        ReturnLength
    );

    if (status != STATUS_SUCCESS) {
        return status;
    }

    auto* pCurrent = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
    PSYSTEM_PROCESS_INFORMATION pPrevious = nullptr;

    while (true) {
        bool ocultar = false;

        if (pCurrent->ImageName.Buffer && pCurrent->ImageName.Length > 0) {
            std::wstring_view processName(
                pCurrent->ImageName.Buffer,
                pCurrent->ImageName.Length / sizeof(wchar_t)
            );

            ocultar = IsSecurityProcessName(processName);

            if (ocultar) {
#ifdef _DEBUG
                LOG(L"Ocultando processo: %.*s (PID: %lu)", 
                    (int)processName.length(),
                    processName.data(),
                    (DWORD)(ULONG_PTR)pCurrent->UniqueProcessId);
#endif
            }
        }

        if (ocultar) {
            if (pPrevious) {
                if (pCurrent->NextEntryOffset != 0) {
                    pPrevious->NextEntryOffset += pCurrent->NextEntryOffset;
                } else {
                    pPrevious->NextEntryOffset = 0;
                    break;
                }
            } else {
                if (pCurrent->NextEntryOffset != 0) {
                    // move o início da lista (primeiro processo oculto)
                    auto* next = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)pCurrent + pCurrent->NextEntryOffset);
                    memmove(pCurrent, next, SystemInformationLength - ((ULONG_PTR)next - (ULONG_PTR)SystemInformation));
                    continue; // reavalie o novo primeiro processo
                } else {
                    break;
                }
            }
        } else {
            pPrevious = pCurrent;
        }

        if (pCurrent->NextEntryOffset == 0)
            break;

        pCurrent = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)pCurrent + pCurrent->NextEntryOffset);
    }

    return status;
}

// =========================
//   Init / DllMain
// =========================
#define CREATE_HOOK(module, func, hook, original) \
    if (MH_CreateHookApi(module, func, reinterpret_cast<LPVOID>(hook), reinterpret_cast<LPVOID*>(&(original))) != MH_OK) return FALSE;

DWORD WINAPI InitHookThread(LPVOID) {
    logger::Init();
    InitializeCriticalSection(&g_cs);
    InitSelfModule(g_hSelf);
    
    // // Config por ambiente (opcional: defina no Loader ou no sistema)
    // g_cfg.swallow_int3        = env_on(L"PS_SWALLOW_INT3");        // 0 (padrão)
    // g_cfg.swallow_ods         = env_on(L"PS_SWALLOW_ODS");         // 0 (padrão)
    // g_cfg.force_write_on_prot = env_on(L"PS_FORCE_WRITE");         // 0 (padrão)
    // g_cfg.block_self_kill     = !env_on(L"PS_ALLOW_SELF_KILL");    // 1 (padrão)
    // g_cfg.hide_debug_flags    = !env_on(L"PS_NO_HIDE_DEBUG");      // 1 (padrão)

    // // Só registra o VEH se solicitado
    // if (g_cfg.swallow_int3) {
        AddVectoredExceptionHandler(1, VectoredAntiDebugHandler);
    //     LOG("VEH instalado (swallow_int3=ON) — cuidado: isso mascara INT3 do debugger.");
    // } else {
    //     LOG("VEH DESLIGADO (padrão).");
    // }

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
    CREATE_HOOK(L"kernel32.dll", "CreateToolhelp32Snapshot", HookedCreateToolhelp32Snapshot, originalCreateToolhelp32Snapshot);
    CREATE_HOOK(L"kernel32.dll", "Process32FirstW", HookedProcess32FirstW, originalProcess32FirstW);
    CREATE_HOOK(L"kernel32.dll", "Process32NextW", HookedProcess32NextW, originalProcess32NextW);
    CREATE_HOOK(L"kernel32.dll", "OutputDebugStringW", HookedOutputDebugStringW, originalOutputDebugStringW);

    // {
    //     LPVOID origTmp = nullptr;
    //     if (MH_CreateHookApi(L"KernelBase.dll", "OutputDebugStringW",
    //                         reinterpret_cast<LPVOID>(HookedOutputDebugStringW),
    //                         reinterpret_cast<LPVOID*>(&origTmp)) == MH_OK) {
    //         if (!originalOutputDebugStringW) originalOutputDebugStringW = (pOutputDebugStringW)origTmp;
    //         LOG("Hook de OutputDebugStringW em KernelBase.dll também instalado.");
    //     }
    // }
    LOG("=============================kernelbase==================================");
    CREATE_HOOK(L"kernelbase.dll", "ReadProcessMemory", HookedReadProcessMemory, originalReadProcessMemoryKB);
    CREATE_HOOK(L"kernelbase.dll", "SetUnhandledExceptionFilter", HookedSetUnhandledExceptionFilter, originalSetUnhandledExceptionFilterKB);
    CREATE_HOOK(L"kernelbase.dll", "VirtualProtect", HookedVirtualProtect, originalVirtualProtectKB);

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
        // ZwQueryInformationProcess
        // HideFromDebugger
        // NtQueryObject

        // ReadProcessMemory

        // VirtualQueryEx

        // GetModuleHandle / GetModuleInformation

        // EnumProcessModules

        if (!CHR("NtQuerySystemInformation", (LPVOID)HookedNtQuerySystemInformation, (LPVOID*)&originalNtQuerySystemInformation)) return FALSE;
        if (!CHR("NtQueryInformationProcess",(LPVOID)HookedNtQueryInformationProcess, (LPVOID*)&originalNtQueryInformationProcess)) return FALSE;
        if (!CHR("NtSetInformationThread",   (LPVOID)HookedNtSetInformationThread,   (LPVOID*)&originalNtSetInformationThread))   return FALSE;
        if (!CHR("NtReadVirtualMemory",      (LPVOID)HookedNtReadVirtualMemory,      (LPVOID*)&originalNtReadVirtualMemory))      return FALSE;
        if (!CHR("NtCreateThreadEx",         (LPVOID)HookedNtCreateThreadEx,         (LPVOID*)&originalNtCreateThreadEx))         return FALSE;
        if (!CHR("NtOpenProcess",            (LPVOID)HookedNtOpenProcess,            (LPVOID*)&originalNtOpenProcess))            return FALSE;
        if (!CHR("NtQueueApcThread",         (LPVOID)HookedNtQueueApcThread,         (LPVOID*)&originalNtQueueApcThread))         return FALSE;        
        if (!CHR("NtTerminateProcess",       (LPVOID)HookedNtTerminateProcess,       (LPVOID*)&originalNtTerminateProcess))       return FALSE;
        if (!CHR("NtWriteVirtualMemory",     (LPVOID)HookedNtWriteVirtualMemory,     (LPVOID*)&originalNtWriteVirtualMemory))     return FALSE;
        // if (!CHR("NtAllocateVirtualMemory",  (LPVOID)HookedNtAllocateVirtualMemory,  (LPVOID*)&originalNtAllocateVirtualMemory))  return FALSE;
        // if (!CHR("NtProtectVirtualMemory",   (LPVOID)HookedNtProtectVirtualMemory,   (LPVOID*)&originalNtProtectVirtualMemory))   return FALSE;
        // if (!CHR("NtQueryVirtualMemory",     (LPVOID)HookedNtQueryVirtualMemory,     (LPVOID*)&originalNtQueryVirtualMemory))     return FALSE;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        LOG("MH_EnableHook(MH_ALL_HOOKS) falhou!");
        return FALSE;
    }

    LOG("=============================Hooks ativados com sucesso============================");
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
        g_hSelf = hModule;                       // guarde seu HMODULE
        DisableThreadLibraryCalls(hModule);      // menos callbacks
        {
            HANDLE th = CreateThread(nullptr, 0, InitHookThread, nullptr, 0, nullptr);
            if (th) CloseHandle(th);
        }
        break;
    case DLL_PROCESS_DETACH:
        UninstallAllHooks();                     // limpe os hooks
        break;
    }
    return TRUE;
}