#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <tlhelp32.h> // Para a "foto" dos processos
#include <filesystem> // Para checar se a DLL existe

// Função para obter o ID de um processo pelo seu nome.
DWORD GetProcessIdByName(const std::wstring& processName) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    // Cria uma "foto" de todos os processos no sistema.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            // Compara o nome do processo atual com o nome alvo.
            if (std::wstring(entry.szExeFile) == processName) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0; // Retorna 0 se não encontrar.
}

// Função que realiza a injeção da DLL no processo alvo.
bool InjectDLL(DWORD processId, const std::string& dllPath) {
    // 1. Obter um "handle" para o processo alvo com permissões completas.
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Erro: Nao foi possivel abrir o processo alvo. Codigo: " << GetLastError() << std::endl;
        return false;
    }

    // 2. Alocar memória dentro do processo alvo para guardar o caminho da nossa DLL.
    LPVOID pDllPathAddress = VirtualAllocEx(hProcess, NULL, dllPath.length() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pDllPathAddress == NULL) {
        std::cerr << "Erro: Nao foi possivel alocar memoria no processo alvo. Codigo: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // 3. Escrever o caminho da DLL na memória que acabamos de alocar.
    if (!WriteProcessMemory(hProcess, pDllPathAddress, dllPath.c_str(), dllPath.length() + 1, NULL)) {
        std::cerr << "Erro: Nao foi possivel escrever na memoria do processo alvo. Codigo: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 4. Obter o endereço da função LoadLibraryA, que existe em todos os processos.
    // O endereço dela é o mesmo em todos os processos para a mesma sessão do Windows.
    LPVOID pLoadLibraryA = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (pLoadLibraryA == NULL) {
        std::cerr << "Erro: Nao foi possivel encontrar o endereco de LoadLibraryA. Codigo: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 5. Criar uma nova thread DENTRO do processo alvo.
    // Essa thread irá executar LoadLibraryA, passando o caminho da nossa DLL como argumento.
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pDllPathAddress, 0, NULL);
    if (hRemoteThread == NULL) {
        std::cerr << "Erro: Nao foi possivel criar a thread remota. Codigo: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    std::cout << "Thread remota criada. Aguardando a execucao..." << std::endl;
    WaitForSingleObject(hRemoteThread, INFINITE); // Espera a thread terminar

    // Limpeza
    CloseHandle(hRemoteThread);
    VirtualFreeEx(hProcess, pDllPathAddress, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

int main() {
    // --- CONFIGURACOES ---
    const std::wstring targetProcessName = L"monitor_antidebug_plus.exe"; // <--- COLOQUE O NOME DO SEU EXE AQUI
    const std::string dllName = "hook_antidebug_duplo_x64.dll";     // <--- COLOQUE O NOME DA SUA DLL AQUI

    // Pega o caminho absoluto da DLL (ela deve estar na mesma pasta que o injetor)
    char currentDir[MAX_PATH];
    GetModuleFileNameA(NULL, currentDir, MAX_PATH);
    std::string::size_type pos = std::string(currentDir).find_last_of("\\/");
    std::string dllPath = std::string(currentDir).substr(0, pos) + "\\" + dllName;

    if (!std::filesystem::exists(dllPath)) {
        std::cerr << "ERRO FATAL: A DLL '" << dllName << "' nao foi encontrada na pasta do injetor." << std::endl;
        system("pause");
        return 1;
    }

    DWORD injectedPID = 0; // Guarda o PID do processo já injetado

    std::cout << "Monitorando processos..." << std::endl;
    std::cout << "Alvo: " << std::string(targetProcessName.begin(), targetProcessName.end()) << std::endl;
    std::cout << "DLL: " << dllPath << std::endl;
    std::cout << "Pressione Ctrl+C para sair." << std::endl;

    while (true) {
        DWORD currentPID = GetProcessIdByName(targetProcessName);

        if (currentPID != 0 && currentPID != injectedPID) {
            std::cout << "\nProcesso alvo encontrado! PID: " << currentPID << std::endl;
            std::cout << "Tentando injetar a DLL..." << std::endl;
            
            // Pausa de segurança para garantir que o processo esteja inicializado
            Sleep(1000); 

            if (InjectDLL(currentPID, dllPath)) {
                std::cout << "[SUCESSO] DLL injetada com sucesso no processo " << currentPID << std::endl;
                injectedPID = currentPID; // Marca como injetado
                std::cout << "\nMonitorando novamente..." << std::endl;
            } else {
                std::cerr << "[FALHA] A injecao da DLL falhou." << std::endl;
                // Poderíamos marcar como falho para não tentar de novo, mas vamos deixar tentar de novo caso o processo reinicie.
            }
        } else if (currentPID == 0 && injectedPID != 0) {
            // O processo que foi injetado foi fechado, reseta o estado.
            std::cout << "\nProcesso alvo foi fechado. Resetando e aguardando nova instancia." << std::endl;
            injectedPID = 0;
        }

        // Espera 2 segundos antes de checar novamente
        Sleep(2000);
    }

    return 0;
}