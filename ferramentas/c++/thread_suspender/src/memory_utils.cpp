#include "memory_utils.hpp"
#include <string>
#include <sstream> // para std::wstringstream
#include <iomanip> // para std::hex

std::vector<MemoryRegionInfo> scanMemoryRegions(HANDLE hProcess) {
    std::vector<MemoryRegionInfo> regions;
    if (!hProcess) return regions;

    uintptr_t address = 0;
    MEMORY_BASIC_INFORMATION mbi{};

    // Itera por todo o espaço de memória do processo
    while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi)) != 0) {
        MemoryRegionInfo region;
        region.baseAddress = (uintptr_t)mbi.BaseAddress;
        region.regionSize = mbi.RegionSize;
        region.state = mbi.State;
        region.protect = mbi.Protect;
        region.type = mbi.Type;
        regions.push_back(region);

        // Pula para a próxima região de memória
        address += mbi.RegionSize;
    }

    return regions;
}

std::wstring protectionToString(DWORD protect) {
    if (protect == 0) return L"---";
    if (protect & PAGE_NOACCESS) return L"No Access";
    if (protect & PAGE_GUARD) return L"[G] ";

    std::wstring s;
    if (protect & PAGE_EXECUTE) s += L"X"; else s += L"-";
    if (protect & PAGE_READWRITE) s += L"RW";
    else if (protect & PAGE_WRITECOPY) s += L"WC";
    else if (protect & PAGE_READONLY) s += L"R-";
    else s += L"--";

    return s;
}