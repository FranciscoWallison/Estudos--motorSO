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



# DUMP COM [pe-sieve](https://github.com/hasherezade/pe-sieve)


```bash
.\pe-sieve32.exe /pid 20380 /refl /data 4 /threads `
  /dmode 3 /imp 5 /ofilter 0 /dir ".\dump_20380"
```


Deposi que gerar o dump renomeie 400000.Game.exe para Game_dump.exe antes de abrir em IDA/Ghidra.



| Campo                | Valor | Significado prático                                                                        |
| -------------------- | ----- | ------------------------------------------------------------------------------------------ |
| **Total scanned**    | 120   | DLLs + blobs mapeados                                                                      |
| **Hooked**           | 7     | Funções sobrescritas (provavelmente `aswhook.dll` do AntiVirus + interceptações do anticheat) |
| **Hdrs Modified**    | 2     | Cabeçalhos PE corrompidos/stripados, comum em autoproteção                                 |
| **Implanted**        | 2 SHC | Dois blocos de shellcode injetado (salvos em `*.shc`)                                      |
| **Total suspicious** | 11    | Soma dos itens que o pE-Sieve marcou como anômalos                                         |


| Passo                                                                  | Comando/sugestão                                                                                                                              | Por quê                                                                           |
| ---------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| 1. Suspenda todas as threads do alvo na hora do dump                   | `Process Hacker ➜ botão direito ➜ Suspend` ou `pe-sieve32.exe /pid 20380 /pause 1`                                                            | Evita que o anticheat troque proteções durante a leitura                          |
| 2. Force leitura de páginas inacessíveis                               | `pe-sieve32.exe /pid 20380 /ofilter 2 /force_read 1`                                                                                          | Tenta ler mesmo em páginas **NOACCESS** (usa `NtProtectVirtualMemory` temporário) |
| 3. Capture só módulos PE válidos                                       | `... /mfilter 1`                                                                                                                              | Evita perder tempo em `.des` ou `.asi` não-PE                                     |
| 4. Se falhar, deixe o próprio Gepard inicializar **antes** de anexar   | Inicie o jogo, aguarde chegar à seleção de personagem, **depois** rode o pE-Sieve                                                             | Após checksums iniciais, algumas páginas voltam a ser **READONLY**                |
| 5. Use versão *devel* do pE-Sieve (≥0.3.5-beta)                        | Build com `/READ_OUT_OF_BOUNDS` habilitado                                                                                                    | Essa flag ignora tamanho errado de seção ao reconstruir headers                   |
| 6. Como alternativa, use **ScyllaHide + Scylla** num debugger (x64dbg) | O plugin *ScyllaHide* mascara chamadas `IsDebuggerPresent`, etc., e o Scylla usa *Import Rebuilder* que às vezes consegue onde pE-Sieve falha |                                                                                   |
---
# Guia de Análise e Depuração de Dumps

## 1 · Fluxo Passo-a-Passo

| # | Etapa | Ferramentas | Procedimento Resumido | Observações |
|---|-------|-------------|-----------------------|-------------|
| 1 | **Validar dump** | **Ghidra** | Importar como **PE with imported symbols**; conferir seções `.text`, `.rdata`, etc. | Se algo faltar, siga para a etapa 2. |
| 2 | **Reconstruir Import Table** | **pE-Bear**&nbsp;/&nbsp;**Scylla** | Rodar sobre o dump com as mesmas flags (`/R1`, `/R2`, …) usadas no dump. | Necessário apenas se a Import Table estiver vazia. |
| 3 | **Revisar hooks (`ntdll.dll`)** | **IDA** + **BinDiff** | Comparar `ntdll` do dump com a cópia limpa do sistema. | Ignore hooks de antivírus; foco nos que apontam para `cps.dll` ou arquivos `*.shc` (Gepard). |
| 4 | **Analisar shellcodes (`*.shc`)** | **Cutter**&nbsp;/&nbsp;**IDA** | Abrir como *Raw binary* (base 0) e procurar APIs `OpenProcess`, `VirtualProtect`, `ReadProcessMemory`. | Cada arquivo ≈ 6 – 8 KB; chamadas a essas APIs indicam anti-debug. |
| 5 | **Obter DLLs limpas** | VM limpa + dumper | Repetir o dump em uma VM recém-instalada sem antivírus. | Garante cópias originais das DLLs para comparação. |
| 6 | **Automatizar diffs** | **LIEF**, **Diaphora** | `lief --diff original.dll patched.dll` ou plugin Diaphora (IDA). | Saída byte-a-byte ou *scoring* rápido de funções. |

---

## 2 · Atalhos de Ferramentas

| Objetivo | Ferramentas | Uso Rápido / Dicas |
|----------|-------------|--------------------|
| Desempacotar e analisar o executável | **Ghidra** (Auto-analysis) | Se faltar importações, reabra com **`/imp 5`** para análise agressiva. |
| Unhook temporário | `hollow-unhook.py` • **x64dbg** (*byte-patch manual*) | Faça somente em VM isolada - pode quebrar o anticheat. |
| Rodar jogo com dump limpo | **Process Hollow** | Injetar `400000.game.exe` no processo suspenso → depuração sem Gepard. |
| Comparar com executável original | **CFF Explorer** • **Detect It Easy** | Verifique se o packer alterou seções, timestamps ou assinaturas. |

---

> ⚠️ **Boa prática de segurança:** execute todas as etapas em máquinas virtuais descartáveis, salve *snapshots* antes das modificações e respeite as licenças de software.


## Verificando hooks
x64dbg
Fila > Open ➜ escolha o executável 400000.game.exe (modo “rebase at load”).
Use o plugin ScyllaHide para evitar detecção de debugger.

No painel Symbols, pesquise pelas funções que aparecem nos hooks do JSON (ex.: NtOpenProcess, LoadLibraryA).

Navegue até o endereço informado e confirme se há um JMP rel32 para fora do módulo.

Se o salto for para aswhook.dll (antivirus) ou npggNT.des, isso explica a contagem “Hooked 7”.