#include "ntdll_utils.hpp"

PFN_NtQueryInformationThread resolveNtQueryInformationThread() {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) hNtdll = LoadLibraryW(L"ntdll.dll");
    if (!hNtdll) return nullptr;

    auto p = reinterpret_cast<PFN_NtQueryInformationThread>(
        GetProcAddress(hNtdll, "NtQueryInformationThread"));
    return p;
}

bool queryThreadStartAddress(HANDLE hThread, void** outStart) {
    static PFN_NtQueryInformationThread NtQueryInformationThread = resolveNtQueryInformationThread();
    if (!NtQueryInformationThread) return false;

    ULONG retLen = 0;
    PVOID startAddr = nullptr;
    NTSTATUS st = NtQueryInformationThread(
        hThread,
        ThreadQuerySetWin32StartAddress,
        &startAddr,
        sizeof(startAddr),
        &retLen
    );
    if (st == 0 /* STATUS_SUCCESS */) {
        if (outStart) *outStart = startAddr;
        return true;
    }
    return false;
}

bool getThreadCurrentAddress(HANDLE hThread, uintptr_t& outAddr) {
    CONTEXT context{};
    // É crucial definir ContextFlags para dizer à API quais partes da estrutura queremos
    context.ContextFlags = CONTEXT_CONTROL;

    if (GetThreadContext(hThread, &context)) {
        // Para x64, o ponteiro de instrução está no registrador Rip
        outAddr = context.Rip;
        return true;
    }

    return false;
}