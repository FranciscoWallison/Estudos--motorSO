// monitor_antidebug_plus.cpp
#include <windows.h>
#include <conio.h>      // _kbhit, _getch
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

static std::string now_hms() {
    using clock = std::chrono::system_clock;
    auto t = clock::to_time_t(clock::now());
    std::tm local{};
    localtime_s(&local, &t);
    std::ostringstream os;
    os << std::setfill('0') << std::setw(2) << local.tm_hour
       << ":" << std::setw(2) << local.tm_min
       << ":" << std::setw(2) << local.tm_sec;
    return os.str();
}

int main() {
    // opcional: console em UTF-8
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "Monitorando IsDebuggerPresent() e CheckRemoteDebuggerPresent()...\n";
    std::cout << "Pressione ENTER para sair.\n\n";

    // Configurações
    const DWORD poll_interval_ms = 200;     // frequência de checagem
    const int   periodic_log_s   = 5;       // log periódico mesmo sem mudança

    // Estados anteriores (forçam primeiro print)
    BOOL last_isdbg   = -1;
    BOOL last_remote  = -1;

    auto last_periodic = std::chrono::steady_clock::now() - std::chrono::seconds(periodic_log_s);

    while (true) {
        // Só sair com ENTER
        if (_kbhit()) {
            int ch = _getch();              // não ecoa no console
            if (ch == '\r') {               // ENTER em Windows é '\r' (13)
                break;
            }
            // ignora outras teclas
        }

        // Leitura dos estados
        BOOL cur_isdbg = IsDebuggerPresent();

        BOOL cur_remote = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &cur_remote);

        // Log se houve mudança
        if (cur_isdbg != last_isdbg || cur_remote != last_remote) {
            std::cout << "[" << now_hms() << "] "
                      << "IsDebuggerPresent=" << (cur_isdbg ? "true" : "false")
                      << " | RemoteDebugger=" << (cur_remote ? "true" : "false")
                      << std::endl;

            last_isdbg  = cur_isdbg;
            last_remote = cur_remote;
            last_periodic = std::chrono::steady_clock::now(); // reinicia janela do periódico
        }

        // Log periódico mesmo sem mudança
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_periodic).count() >= periodic_log_s) {
            std::cout << "[" << now_hms() << "] "
                      << "(periodic) IsDebuggerPresent=" << (cur_isdbg ? "true" : "false")
                      << " | RemoteDebugger=" << (cur_remote ? "true" : "false")
                      << std::endl;

            last_periodic = now;
            // não atualiza last_isdbg/last_remote aqui, para que mudanças reais continuem aparecendo
        }

        Sleep(poll_interval_ms);
    }

    std::cout << "\nEncerrando. Ate mais!\n";
    return 0;
}
