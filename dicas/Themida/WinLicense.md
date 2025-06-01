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

