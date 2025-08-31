#pragma once
#include <windows.h>
#include <vector>
#include <string>

// Estrutura para guardar as informações de cada região de memória
struct MemoryRegionInfo {
    uintptr_t baseAddress;
    size_t regionSize;
    DWORD state;       // MEM_COMMIT, MEM_RESERVE, MEM_FREE
    DWORD protect;     // PAGE_READWRITE, PAGE_EXECUTE_READ, etc.
    DWORD type;        // MEM_IMAGE, MEM_MAPPED, MEM_PRIVATE
};

// Função que irá varrer a memória do processo e retornar um vetor de regiões
std::vector<MemoryRegionInfo> scanMemoryRegions(HANDLE hProcess);

// Função auxiliar para converter permissões em string legível
std::wstring protectionToString(DWORD protect);