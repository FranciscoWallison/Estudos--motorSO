#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <iomanip>

#include "privilege.hpp"
#include "process_utils.hpp"
#include "thread_utils.hpp"
#include "ntdll_utils.hpp"
#include "memory_utils.hpp" 


static void printAddrAsModuleOffset(const ModuleInfo& mod, uintptr_t addr, const std::wstring& exeName) {
    if (addr >= mod.base && addr < (mod.base + mod.size)) {
        uintptr_t offset = addr - mod.base;
        std::wcout << exeName << L"+0x" << std::hex << offset << std::dec;
    } else {
        std::wcout << L"0x" << std::hex << addr << std::dec;
    }
}
static const ModuleInfo* findModuleForAddress(uintptr_t addr, const std::vector<ModuleInfo>& modules) {
    for (const auto& mod : modules) {
        if (addr >= mod.base && addr < (mod.base + mod.size)) {
            return &mod;
        }
    }
    return nullptr;
}
int wmain(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        std::wcout << L"Uso: " << argv[0] << L" <nome_do_executavel.exe> [--keep-main] [--ignore-tids 1,2] [--timeout 120]\n";
        return 1;
    }

    // --- Parsing de argumentos (sem alterações) ---
    std::wstring targetExe = argv[1];
    bool keepMain = true;
    int timeoutSec = 0;
    std::unordered_set<DWORD> ignoreTids;

    for (int i = 2; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--keep-main") keepMain = true;
        else if (arg == L"--no-keep-main") keepMain = false;
        else if (arg == L"--timeout" && i + 1 < argc) timeoutSec = _wtoi(argv[++i]);
        else if (arg == L"--ignore-tids" && i + 1 < argc) {
            std::wstring list = argv[++i];
            size_t pos = 0;
            while (pos < list.size()) {
                size_t comma = list.find(L',', pos);
                auto token = list.substr(pos, (comma == std::wstring::npos ? list.size() : comma) - pos);
                if (!token.empty()) ignoreTids.insert(static_cast<DWORD>(_wtoi(token.c_str())));
                if (comma == std::wstring::npos) break;
                pos = comma + 1;
            }
        }
    }

    std::wcout << L"[*] SeDebugPrivilege: " << (enableDebugPrivilege() ? L"ok" : L"falhou (prosseguindo)") << L"\n";

    // --- Encontrar o processo (sem alterações) ---
    std::wcout << L"[*] Aguardando processo '" << targetExe << L"'...\n";
    DWORD pid = 0, waited = 0;
    while (!pid) {
        pid = getPidByName(targetExe);
        if (pid) break;
        Sleep(500);
        waited += 500;
        if (timeoutSec && waited >= (DWORD)timeoutSec * 1000) {
            std::wcerr << L"[!] Timeout aguardando " << targetExe << L"\n";
            return 2;
        }
    }
    std::wcout << L"[+] PID: " << pid << L"\n";

    ModuleInfo mainMod{};
    if (!getMainModule(pid, mainMod)) {
        std::wcerr << L"[!] Nao foi possivel obter o modulo principal\n";
        return 4;
    }
    std::wcout << L"[*] Main: " << mainMod.name << L" (" << mainMod.path << L") base=0x"
               << std::hex << mainMod.base << L" size=0x" << mainMod.size << std::dec << L"\n";
    
    // --- MUDANÇA PRINCIPAL AQUI ---
    // Agora o programa espera por você antes de suspender qualquer coisa.
    std::wcout << L"\n[!] Processo encontrado. Pressione Enter no momento desejado para congelar e inspecionar as threads...\n";
    std::wcin.get();
    // --- FIM DA MUDANÇA ---


    // O resto do código agora só executa depois que você apertar Enter
    auto tids = listThreadsOf(pid);
    if (tids.empty()) {
        std::wcerr << L"[!] Nenhuma thread encontrada\n";
        return 7;
    }
    std::wcout << L"[*] " << tids.size() << L" threads encontradas. Suspendendo todas...\n";

    std::vector<HANDLE> thandles; thandles.reserve(tids.size());
    for (DWORD tid : tids) {
        HANDLE hThread{};
        DWORD err=0;
        if (!suspendThreadById(tid, hThread, err)) {
            std::wcerr << L"    - Open/Suspend TID " << tid << L" falhou (" << err << L")\n";
            thandles.push_back(nullptr);
            continue;
        }
        thandles.push_back(hThread);
        std::wcout << L"    - Suspenso TID " << tid << L"\n";
    }

    std::wcout << L"\n[!] Processo 'congelado'. Inspecionando ponteiro de instrucao atual (IP)...\n";

    // Pega a lista de TODOS os módulos
    auto allModules = listAllModules(pid);

    // Salva as informações de cada thread
    struct ThreadSnapshot {
        DWORD tid;
        uintptr_t ip;
        const ModuleInfo* module = nullptr;
    };
    std::vector<ThreadSnapshot> snapshots;

    for (size_t i = 0; i < tids.size(); ++i) {
        HANDLE hThread = thandles[i];
        if (!hThread) continue;
        
        ThreadSnapshot snap;
        snap.tid = tids[i];
        if (getThreadCurrentAddress(hThread, snap.ip)) {
            snap.module = findModuleForAddress(snap.ip, allModules);
        }
        snapshots.push_back(snap);

        // Imprime as informações detalhadas
        std::wcout << L"    - TID " << std::setw(5) << snap.tid << L" IP=";
        if (snap.module) {
            std::wcout << snap.module->name << L"+0x" << std::hex << (snap.ip - snap.module->base);
        } else {
            std::wcout << L"0x" << std::hex << snap.ip;
        }
        std::wcout << std::dec << L"\n";
    }

    std::wcout << L"\n[*] Digite os TIDs que voce deseja MANTER SUSPENSOS (separados por virgula, ex: 123,456) e pressione Enter:\n> ";
    std::wstring input;
    std::getline(std::wcin, input);

    std::unordered_set<DWORD> tidsToKeepSuspended;
    std::wstringstream ss(input);
    std::wstring token;
    while (std::getline(ss, token, L',')) {
        if (!token.empty()) {
            tidsToKeepSuspended.insert(static_cast<DWORD>(_wtoi(token.c_str())));
        }
    }

    std::wcout << L"\n[*] Retomando seletivamente...\n";
        std::wcout << L"\n[*] Retomando threads seletivamente...\n";
    for (size_t i = 0; i < tids.size(); ++i) {
        DWORD tid = tids[i];
        HANDLE& hThread = thandles[i];
        if (!hThread) continue;

        // A decisão agora é baseada na sua escolha!
        if (tidsToKeepSuspended.count(tid)) {
            std::wcout << L"    - MANTENDO SUSPENSA TID " << tid << L"\n";
        } else {
            if (!resumeAndClose(hThread)) {
                std::wcerr << L"    - Resume TID " << tid << L" falhou (" << GetLastError() << L")\n";
            } else {
                std::wcout << L"    - Retomada TID " << tid << L"\n";
            }
        }
    }
    

    std::wcout << L"\n[+] Pronto. Pressione Enter para liberar TUDO e sair...\n";
    std::wcin.get();

    for (auto& h : thandles) {
        if (h) resumeAndClose(h);
    }
     // --- VARREDURA DE MEMÓRIA ---
    std::wcout << L"\n[*] Iniciando varredura de memoria do processo...\n";
    HANDLE hProcForScan = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcForScan) {
        auto memoryRegions = scanMemoryRegions(hProcForScan);
        std::wcout << L"    - Encontradas " << memoryRegions.size() << L" regioes de memoria.\n";
        
        // Imprime algumas informações de exemplo
        std::wcout << L"    Base Address       | Size       | Protection\n";
        std::wcout << L"    -------------------+------------+--------------\n";
        for (const auto& region : memoryRegions) {
            // Imprime apenas regiões de memória que estão alocadas ("committed")
            if (region.state == MEM_COMMIT) {
                 std::wcout << L"    0x" << std::hex << std::setw(16) << std::setfill(L'0') << region.baseAddress
                           << L" | 0x" << std::setw(8) << std::setfill(L'0') << region.regionSize
                           << L" | " << protectionToString(region.protect) << std::dec << L"\n";
            }
        }
        CloseHandle(hProcForScan);
    } else {
        std::wcerr << L"[!] Falha ao abrir o processo para varredura de memoria (Erro: " << GetLastError() << L")\n";
    }
    // --- FIM DA NOVA SEÇÃO ---

    return 0;
}