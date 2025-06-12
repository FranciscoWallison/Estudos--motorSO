### 🔐 **O que é Themida/WinLicense?**

Themida e WinLicense são **empacotadores comerciais usados para proteger executáveis** (EXE e DLL), normalmente usados por softwares comerciais (ou malware) para evitar:

* Engenharia reversa
* Debug
* Crack
* Análise por antivírus

Esses empacotadores usam criptografia, compressão e ofuscação para proteger o código original.

---

### 🛠️ **O que essa ferramenta faz?**

Ela **executa o binário protegido dinamicamente** (como um debugger automatizado) para:

* Recuperar o código original desprotegido em tempo de execução.
* Descobrir o **OEP (Original Entry Point)** — onde o programa real começa.
* Reconstruir a **Import Table** — funções de APIs chamadas pelo programa.
* Suporta executáveis 32 e 64 bits (EXE e DLL).
* Também lida com arquivos `.NET` EXE.

---

### ⚠️ **Atenção:**

* **Executa o EXE real** – use **em máquina virtual** se não tiver certeza do que o binário faz!
* **Python 32 bits** é necessário se o alvo for 32 bits.
* Não funciona bem com DLLs .NET.
* Nem sempre o dump final é executável, mas serve para análise em ferramentas como Ghidra ou IDA.

---

### ✅ **Como usar (modo simples):**

1. Baixe o `unlicense.exe` da seção de [Releases do GitHub](https://github.com/ergrelet/unlicense/releases)
2. **Arraste o EXE protegido** para o `unlicense.exe` correto (32 ou 64 bits)
3. Ele irá executar, pausar no OEP, dump do código real, e reconstruir a Import Table.

---

### 💻 **Como usar pelo terminal (CLI):**

```bash
unlicense.exe GAme.exe --verbose
```

Outros flags úteis:

* `--pause_on_oep`: pausa na hora que achar o OEP
* `--no_imports`: não tenta reconstruir imports
* `--force_oep=0x401000`: força OEP manual (se você já sabe)
* `--timeout=15`: aumenta o tempo de execução antes do dump

---

### 📦 **Instalação via pip (modo avançado):**

```bash
pip install git+https://github.com/ergrelet/unlicense.git
```

Ou clone o repositório e use:

```bash
python -m unlicense GAme.exe
```

---

### 📎 **Tecnologias usadas:**

* **Frida**: framework para injetar scripts durante execução
* **Scylla**: para reconstrução de Import Tables
* **Python**: automação de dumping e análise

---

### 🧠 **Quando usar essa ferramenta:**

* Quando o executável está fortemente protegido com **Themida ou WinLicense**
* Você precisa extrair o código real para análise com Ghidra/IDA
* Softwares empacotados não funcionam em debuggers (como x64dbg) ou retornam instruções ilegíveis


### Ferramentas (gratuitas ou open-source) que hoje conseguem **desempacotar ou, no mínimo, contornar** proteções Themida/WinLicense 3.x

| Finalidade                             | Ferramenta                                                                                                        | Observações rápidas                                                                                                                       |
| -------------------------------------- | ----------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| **Unpack automático por emulação**     | **Unlicense**                                                                                                     | CLI em Python, lida com 32 / 64 bit, restaura OEP e tabela de importações. Bom ponto de partida para dumps rápidos. ([github.com][1])     |
|                                        | **bobalkkagi**                                                                                                    | Emulador/hooker em Python voltado para Themida 3.1.3; vários modos (fast, hook\_code, hook\_block). ([github.com][2])                     |
|                                        | *Magicmida*                                                                                                       | Auto-unpacker 32-bit; funciona em alguns alvos, mas manutenção irregular (requer conta no ExeTools). ([forum.exetools.com][3])            |
| **Bypass de anti-debug / VMChecks**    | **Themidie (plugin x64dbg)**                                                                                      | Hooka APIs críticas e desarma checagens anti-debug da linha 3.x (x64). Útil antes de fazer dump manual. ([github.com][4])                 |
|                                        | **ScyllaHide**                                                                                                    | Biblioteca anti-anti-debug com perfis prontos (x64dbg, Olly, etc.). Carregue o perfil “Themida/WinLicense” ou “Custom”. ([github.com][5]) |
|                                        | **TitanHide**                                                                                                     | Versão ring-0; só se o alvo detectar debuggers em kernel mode.                                                                            |
| **Dump + reconstrução de importações** | **Scylla / Scylla-x64dbg**                                                                                        | Depois de pausar no OEP, faça “Dump PE” + “Fix Import”.                                                                                   |
|                                        | **PE-sieve**                                                                                                      | Boa para dumps parciais em casos de processos que se auto-deletam.                                                                        |
| **Scripts para x64dbg**                | **OEP/Import finders** no repositório *x64dbg/Scripts*; há um “Themida & VMProtect OEP Finder”. ([github.com][6]) |                                                                                                                                           |
| **Comunidades / tutoriais**            | Tuts4You, RevTeam, ExeTools                                                                                       | Repositórios de scripts, unpack-mes e discussões. ([forum.tuts4you.com][7], [revteam.re][8])                                              |

---

#### Fluxo de trabalho “rápido”

1. **Teste um auto-unpacker**

   ```bash
   # 64-bit
   unlicense.exe protected.exe
   # 32-bit
   unlicense32.exe protected.exe
   ```

   Se o dump rodar, ótimo; senão, parta para o modo manual.

2. **Bypass anti-debug antes de anexar debugger**

   * Copie *Themidie.dll* + *.dp64* e *ScyllaHide* para a pasta *plugins* do x64dbg.
   * Abra x64dbg → Plugins → ScyllaHide → Options → **Kill Anti-Attach** apenas → OK.
   * Plugins → Themidie → Start → selecione o executável. Isso suspende o alvo num ponto seguro para anexo. ([github.com][4])

3. **Encontrar OEP e fazer dump**

   * Quando parar no módulo Themida, siga as instruções do script *OEP Finder* (ou pressione *Run* se estiver usando o script automático). ([github.com][6])
   * No OEP: Scylla → **Dump PE** → **Fix Import**.

4. **Refinar**

   * Caso o binário continue quebrando: recalcule relocations, corrija section flags e verifique TLS callbacks residuais.
   * Para executáveis WinLicense com arquivo de licença, copie o *.wllic* para o mesmo diretório do dump.

---

#### Dicas rápidas

* **VM isolada** – Ferramentas dinâmicas executam o alvo; use snapshot para evitar infecção.
* **32-bit vs 64-bit** – Use Python 32-bit para dump 32-bit com Unlicense; bobalkkagi cobre só alguns builds 64-bit.
* **Versões recentes** – Themida 3.2.3.0 (mar 2025) introduziu pequenas mudanças no stub, mas Unlicense v0.4 e Themidie já cobrem.
* **Limitações** – Proteção por virtualização (VM) ainda exige engenharia manual ou devirtualizadores privados; nenhuma ferramenta open-source faz “devirtualização total” de 3.x.

---

> **Uso ético**: empregar essas técnicas apenas em ambientes de pesquisa, auditoria de segurança ou onde a licença permita engenharia reversa.

[1]: https://github.com/ergrelet/unlicense "GitHub - ergrelet/unlicense: Dynamic unpacker and import fixer for Themida/WinLicense 2.x and 3.x."
[2]: https://github.com/bobalkkagi/bobalkkagi "GitHub - bobalkkagi/bobalkkagi: Themida 3.x unpacking, unwrapping and devirtualization(future)"
[3]: https://forum.exetools.com/showthread.php?t=20466&utm_source=chatgpt.com "Magicmida - Themida unpacker - Exetools"
[4]: https://github.com/VenTaz/Themidie "GitHub - VenTaz/Themidie: x64dbg plugin to bypass Themida 3.x Anti-Debugger / VM / Monitoring programs checks (x64)"
[5]: https://github.com/x64dbg/ScyllaHide?utm_source=chatgpt.com "x64dbg/ScyllaHide: Advanced usermode anti-anti ... - GitHub"
[6]: https://github.com/x64dbg/Scripts/commits?utm_source=chatgpt.com "Commits · x64dbg/Scripts - GitHub"
[7]: https://forum.tuts4you.com/topic/44124-themida-x32-v3040/page/2/?utm_source=chatgpt.com "Themida x32 v3.0.4.0 - Page 2 - UnPackMe - Tuts4You forum"
[8]: https://revteam.re/forum/threads/themida-winlicense-2-x-and-3-x-unpacker.1230/ "Themida/WinLicense 2.x and 3.x Unpacker | RevTeam.Re - Reverse Engineering Team"

