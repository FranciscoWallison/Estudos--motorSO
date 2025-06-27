### O que é o ImHex?

ImHex é um editor hexadecimal open-source voltado a engenharia reversa. Ele combina um visual moderno em Qt com diversos painéis voltados à análise binária — visualização hexadecimal, árvore de estruturas, disassembler, visor de entropia, console de scripts e patcher — tudo em um só programa. O projeto é mantido por **WerWolv** e distribuído gratuitamente para Windows, macOS, Linux (AppImage/Flatpak) e até como versão WebAssembly que roda no navegador. ([imhex.werwolv.net][1])

---

### Diferenciais que fazem a ferramenta brilhar

| Recurso                               | Para que serve                                                                                                                                | Observações                                                                                                                    |
| ------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| **Pattern Language (HexLang)**        | Descrever formatos binários de modo declarativo, similar a C + Kaitai. O parse é feito em tempo-real, exibindo cada campo num painel lateral. | Permite comentários, expressões, enumerações e condicionais; perfeito para pacotes de rede ou cabeçalhos PE. ([github.com][2]) |
| **Inspector dinâmico**                | Seleciona bytes e vê valores em inteiro, float, string, tempo, etc.                                                                           | Útil para sanity-check de offsets rapidamente.                                                                                 |
| **Disassembler integrado (Capstone)** | Clique em “Disassemble selection” e tenha listagem para várias arquiteturas (x86, ARM, MIPS, RISC-V).                                         | Crashes raros ainda existem em capstone-ARM; há issue aberta. ([github.com][3])                                                |
| **Diff & Visualizações**              | Cálculo de entropia, mapa de cores por regiões, comparação de arquivos.                                                                       | Bom para detectar seções compactadas/criptografadas.                                                                           |
| **Sistema de plugins**                | Extensão em C/C++ ou Rust; hooks no pipeline de parsing, views customizadas, comandos do console.                                             | Nightlies já incluem SDK exemplo.                                                                                              |
| **Modo Web (`ImHexWeb`)**             | Interface quase idêntica sem instalar nada, útil para demonstrar scripts em aulas ou artigos.                                                 | Pode ser auto-hospedado. ([github.com][4])                                                                                     |

---

### Novidades recentes (ramo 1.37.x)

* **Hi-DPI e fontes independentes** – Interface escala bem em 4K e é possível adotar fonte monoespaçada diferente só para o editor.
* **Compilação MSVC/ClangCL** – Build Windows mais simples; ajuda quem mantém forks ou quer customizar.
* **Binários ARM64 & WebAssembly** – AppImage nativo para Raspberry Pi/Apple Silicon e o já citado ImHexWeb.
* **Barra de menu nativa no macOS** – Integra melhor com atalho padrão. ([github.com][4])

> **Heads-up:** a versão estável mais nova é **1.37.4**; mantenedores pedem um 1.38 pois o 1.37.4 tem bug de compilação em GCC 14 e pacotes Linux estão em hold. ([github.com][5])

---

### Fluxo de uso típico (exemplo para seu perfil de game hacking)

1. **Abra o executável ou dump de memória** do cliente do jogo (ex.: `game.exe`).
2. Escreva ou importe um *pattern* que descreva o cabeçalho do pacote ou a estrutura do mapa de memória com offsets dinâmicos.
3. Use o painel **Inspector** para confirmar valores críticos (CRC, versão, flags).
4. No **Disassembler**, localize funções de criptografia; copie o offset para definir breakpoints no x64dbg.
5. Via **Patch view**, altere bytes (NOP, JMP) e salve em copiador incremental para reverter rápido.

Esse fluxo evita salto constante entre 010 Editor, Capstone CLI e HxD, centralizando tudo numa única UI.

---

### Instalação & dicas rápidas

* **Windows/macOS**: baixe instalador ou .zip no GitHub Releases ou via Chocolatey/Homebrew.
* **Linux**: AppImage (portable), Flatpak (`flathub org.imhex.ImHex`) ou AUR (`imhex-bin`).
* **Atalhos úteis**:

  * `Ctrl+P` – Paleta de comandos (busque todas funções)
  * `Ctrl+Shift+F` – Busca por sequência (regex, número, texto)
  * `Alt+E` – Alterna endianess da seleção
* **Scripts**: teclas `~` abrem console Lua; ideal para automatizar decodificação ou fuzzing simples.

---

### Pontos fortes & limitações

**✔ Prós**

* Interface coesa, dark-mode amigável de madrugada.
* Pattern Language poderoso, comunidade ativa com repositório público de templates.
* Versão web facilita demonstração didática.

**✘ Contras**

* Consome bastante RAM em arquivos > 1 GB.
* Falta depurador nativo; é apenas visualizador/patcher.
* Alguns *edge-cases* de crash ainda não resolvidos (Capstone/Qt).

---

### Recursos para se aprofundar

* **Docs oficiais & tutoriais** (`Help ▸ Open documentation` ou wiki no GitHub).
* **Repositório de Patterns** para formatos comuns (ROMs, protocolos de rede, firmware). ([github.com][2])
* **Canal Discord** – troque bancos de *patterns*, discuta parsing avançado.
* **Issues do GitHub** – acompanhe progresso de features (ex.: WebAssembly viewer, plugin de decompilação).


```c
/* Padrão ImHex para pacotes de rede do Games (servidores com AntiCheat)
 * Autor: ChatGPT (OpenAI o3)
 * Data: 27-06-2025
 *
 * Fontes consultadas:
 * – Wiki OpenKore sobre subsistema de rede ([openkore.com](https://openkore.com/wiki/network_subsystem))
 * – Listagem de pacotes do rAthena para IDs de referência ([github.com](https://github.com/rathena/rathena/blob/master/doc/packet_interserv.txt))
 *
 * NOTA: A estrutura dos campos varia entre as compilações do servidor; ajuste conforme necessário.
 */

endian little; // Define a ordem dos bytes como little-endian

//─────────────────────────────────────────────────────────────
// Enumerações
//─────────────────────────────────────────────────────────────

/* Lista parcial de identificadores de pacotes (IDs) usados por clientes Renewal.
 * Adicione/remova conforme necessário para a versão do seu servidor.
 */
enum PacketID : uint16 {
    AC_ACCEPT_LOGIN  = 0x0063, // Login aceito (servidor de autenticação -> cliente)
    HC_ACCEPT_ENTER  = 0x0073, // Entrada no servidor de personagens aceita
    CZ_ENTER         = 0x0072, // Entrar no mapa (cliente -> servidor de mapa)
    SMSG_CHAT        = 0x008D, // Mensagem de chat (servidor -> cliente)
};

//─────────────────────────────────────────────────────────────
// Cabeçalho comum presente em todas as mensagens
//─────────────────────────────────────────────────────────────

struct PacketHeader {
    PacketID id;   // Identificador de 2 bytes (little-endian)
    uint16   len;  // Comprimento total do pacote, incluindo o cabeçalho
};

//─────────────────────────────────────────────────────────────
// Definições de payloads específicos (exemplos)
//─────────────────────────────────────────────────────────────

/* 0x0072 – CZ_ENTER (cliente → servidor de mapa: login no mapa)
 * O formato é baseado no kRO 2020. A sua compilação pode ser diferente!
 */
struct CZ_ENTER_PAYLOAD {
    uint32 account_id;
    uint32 char_id;
    uint32 login_token;      // também conhecido como loginID1
    char   map_name[16];     // terminado por nulo, a menos que esteja cheio (sem NUL)
    uint32 client_tick;      // GetTickCount()
    uint16 x;
    uint16 y;
    uint8  sex;              // 0 = masculino, 1 = feminino
};

/* 0x008D – SMSG_CHAT (servidor → cliente: mensagem de chat) */
struct SMSG_CHAT_PAYLOAD {
    uint32 sender_id;
    char   message[$parent.header.len - 8]; // subtrai cabeçalho + sender_id
};

//─────────────────────────────────────────────────────────────
// Despachante que percorre toda a captura / dump binário
//─────────────────────────────────────────────────────────────

/*
 * Este loop "while" de nível superior continua a analisar estruturas Packet
 * até que o cursor (deslocamento atual) atinja o final do arquivo (EOF).
 */

while ($cursor < file_size) {
    PacketHeader header;

    // Direciona a análise do payload dependendo do header.id
    switch (header.id) {
        case PacketID::CZ_ENTER  => CZ_ENTER_PAYLOAD payload;
        case PacketID::SMSG_CHAT => SMSG_CHAT_PAYLOAD payload;
        default                  => uint8 raw[header.len - 4];
    };
};

//─────────────────────────────────────────────────────────────
// Dicas
//─────────────────────────────────────────────────────────────
/*
 * ▸ Se os seus pacotes chegam criptografados (AntiCheat/SX-50), capture-os APÓS
 * a descriptografia, "hookando" a função "recv" no cliente ou fazendo um dump da memória.
 * ▸ Use a "visualização de entropia" do ImHex para identificar blocos comprimidos/empacotados.
 * ▸ Expanda este arquivo: adicione novas structs e casos no "switch" à medida que você
 * faz a engenharia reversa de mais IDs de pacotes (veja a documentação do rAthena para definições).
 */
```