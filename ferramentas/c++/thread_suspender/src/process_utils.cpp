#include "process_utils.hpp"
#include <tlhelp32.h>
#include <vector>

DWORD getPidByName(const std::wstring& exeNameW) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    DWORD pid = 0;

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeNameW.c_str()) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool getMainModule(DWORD pid, ModuleInfo& out) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    MODULEENTRY32W me{}; me.dwSize = sizeof(me);
    if (!Module32FirstW(hSnap, &me)) {
        CloseHandle(hSnap);
        return false;
    }

    out.base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
    out.size = static_cast<size_t>(me.modBaseSize);
    out.name = me.szModule;
    out.path = me.szExePath;

    CloseHandle(hSnap);
    return true;
}

std::vector<ModuleInfo> listAllModules(DWORD pid) {
    std::vector<ModuleInfo> modules;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE) return modules;

    MODULEENTRY32W me{}; me.dwSize = sizeof(me);
    if (Module32FirstW(hSnap, &me)) {
        do {
            ModuleInfo mod;
            mod.base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            mod.size = static_cast<size_t>(me.modBaseSize);
            mod.name = me.szModule;
            mod.path = me.szExePath;
            modules.push_back(mod);
        } while (Module32NextW(hSnap, &me));
    }
    CloseHandle(hSnap);
    return modules;
}