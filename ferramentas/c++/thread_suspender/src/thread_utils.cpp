#include "thread_utils.hpp"
#include <tlhelp32.h>

std::vector<DWORD> listThreadsOf(DWORD pid) {
    std::vector<DWORD> tids;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return tids;

    THREADENTRY32 te{}; te.dwSize = sizeof(te);
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) tids.push_back(te.th32ThreadID);
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
    return tids;
}

bool resumeAndClose(HANDLE& hThread) {
    if (!hThread) return false;
    bool ok = (ResumeThread(hThread) != (DWORD)-1);
    CloseHandle(hThread);
    hThread = nullptr;
    return ok;
}


bool suspendThreadById(DWORD tid, HANDLE& outHandle, DWORD& outErr) {
    outErr = 0; // Zera o erro de saída
    outHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, tid);

    if (!outHandle) { 
        outErr = GetLastError(); 
        return false; 
    }

    // Chama SuspendThread apenas UMA VEZ e verifica se houve erro.
    // O valor de retorno de SuspendThread é o contador de suspensão anterior.
    // Um valor de (DWORD)-1 indica erro.
    if (SuspendThread(outHandle) == (DWORD)-1) {
        outErr = GetLastError(); // Salva o erro de SuspendThread
        CloseHandle(outHandle);    // Limpa o handle que foi aberto
        outHandle = nullptr;       // Garante que o handle de saída não seja usado
        return false;
    }

    // Se chegou aqui, a thread foi suspensa com sucesso
    return true;
}