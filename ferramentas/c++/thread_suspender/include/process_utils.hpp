#pragma once
#include <windows.h>
#include <string>
#include <cstdint>
#include <vector>

struct ModuleInfo {
    std::wstring name;
    std::wstring path;
    uintptr_t base = 0;
    size_t size = 0;
};

DWORD getPidByName(const std::wstring& exeNameW);
bool  getMainModule(DWORD pid, ModuleInfo& out);
std::vector<ModuleInfo> listAllModules(DWORD pid);
