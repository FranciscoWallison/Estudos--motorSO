

### 📍 Palavras Chaves:
> **Snapshot = cópia instantânea do estado atual de algo** (processos, threads, módulos, etc), geralmente usada para análise, depuração ou injeção.

> **Módulos** Em um processo do Windows, módulos são basicamente: Executáveis (.exe) e Bibliotecas dinâmicas (.dll)

> **Handle** É um identificador abstrato (geralmente um número inteiro) usado pelo sistema operacional para referenciar recursos internos [Mais sobre](https://github.com/FranciscoWallison/Estudos---motorSO/tree/main/ferramentas/python/Handle.md)

> **Shellcode** é um bloco de código binário (geralmente em assembly) que é escrito para ser injetado e executado dentro da memória de um processo. Ele é chamado assim porque originalmente era usado para abrir um shell (prompt de comandos) quando executado – daí o nome “shell-code”. Hoje, o termo se expandiu e se refere a qualquer código executável inserido na memória com a intenção de ser rodado, independentemente de abrir um shell ou não.

### 🧪 **Funções da API Win32 – O que cada uma faz?**

| **Função**                 | **Descrição detalhada** |
|---------------------------|--------------------------|
| `OpenProcess`             | Abre um processo em execução com permissões específicas (como leitura, escrita e execução). É o primeiro passo para interagir com outro processo, por exemplo, ao injetar uma DLL ou monitorar sua memória. |
| `CloseHandle`             | Encerra e libera um "handle" (identificador de recurso) que foi aberto anteriormente, como um handle de processo, thread ou arquivo. Importante para evitar vazamentos de memória. |
| `VirtualAllocEx`          | Aloca um bloco de memória **dentro do processo remoto**, ou seja, o processo alvo. Usado para reservar espaço onde você irá escrever dados (como o caminho da DLL). |
| `WriteProcessMemory`      | Escreve dados diretamente na memória de outro processo. Muito utilizado para colocar uma string com o caminho da DLL dentro do processo alvo antes de executar `LoadLibraryA`. |
| `CreateRemoteThread`      | Cria e executa uma nova thread **dentro do processo remoto**. Geralmente usada para chamar `LoadLibraryA` com o caminho da DLL já escrito na memória alocada via `VirtualAllocEx`. |
| `LoadLibraryA`            | Carrega uma DLL em tempo de execução, dado seu nome/caminho. Quando usada remotamente (via `CreateRemoteThread`), carrega sua DLL no processo alvo. |
| `CreateToolhelp32Snapshot`| Cria uma "fotografia" (snapshot) dos processos, threads, módulos (DLLs) ou heaps do sistema. É usada como base para varreduras com `Process32First`, `Module32First`, etc. |
| `Module32First / Module32Next` | Permitem iterar (listar um por um) os módulos carregados dentro de um processo (como DLLs). Muito úteis para verificar se uma DLL específica foi carregada. |
| `WaitForSingleObject`     | Faz seu programa esperar até que um determinado **handle** (como uma thread) seja finalizado. Por exemplo, esperar a thread remota terminar de carregar a DLL antes de continuar. |
________________________________
Esse é o **esqueleto típico de um injetor de DLL**, usado assim:

1. Abre o processo alvo com `OpenProcess`.
2. Usa `VirtualAllocEx` para alocar espaço no processo remoto.
3. Usa `WriteProcessMemory` para escrever o caminho da DLL nesse espaço.
4. Cria uma thread remota com `CreateRemoteThread` que executa `LoadLibraryA`.
5. Espera a thread terminar com `WaitForSingleObject`.
6. Limpa com `CloseHandle`.
___________________________________________________
## 🧠 Constantes e Estruturas – Explicação Técnica

### `PROCESS_ALL_ACCESS = win32con.PROCESS_ALL_ACCESS`

- **O que é**: Permissão total para manipular um processo.
- **Serve para**: Abrir um processo com direito de fazer praticamente tudo (ler, escrever memória, criar thread remota, etc).
- **Usado em**: `OpenProcess`.
- **Obs**: Pode falhar se o processo não tiver permissões adequadas (proteções do Windows como UAC ou anticheat podem impedir).

---

### `MEM_COMMIT = win32con.MEM_COMMIT`

- **O que é**: Instrução para alocar **memória física real** (RAM ou pagefile) para uma área de memória virtual.
- **Serve para**: Deixar a memória utilizável (lida ou escrita).
- **Usado em**: `VirtualAllocEx`.

---

### `MEM_RESERVE = win32con.MEM_RESERVE`

- **O que é**: Reserva um bloco de endereço virtual, **sem alocar memória física ainda**.
- **Serve para**: Preparar uma área que será usada futuramente (pode ser depois "commitada").
- **Usado em**: Também em `VirtualAllocEx`.

---

### `PAGE_EXECUTE_READWRITE = win32con.PAGE_EXECUTE_READWRITE`

- **O que é**: Proteção da memória alocada.
- **Permite**: Executar código, ler e escrever na memória.
- **Usado em**: `VirtualAllocEx`.
- ⚠️ **Cuidado**: É uma das proteções mais perigosas, usada em injeção de DLLs e shellcode.

---

### `NULL = win32con.NULL`

- **O que é**: Valor nulo, equivalente a `0` em C/C++.
- **Serve para**: Indicar ponteiros ou parâmetros opcionais que estão vazios.
- **Usado em**: Quase toda a API do Windows (por exemplo, o segundo parâmetro de `OpenProcess` pode ser `NULL`).

---

### `INVALID_HANDLE_VALUE = -1`

- **O que é**: Valor de retorno que indica **erro** ao abrir ou criar um handle.
- **Usado em**: `CreateToolhelp32Snapshot`, entre outras.
- **Importante**: Sempre teste se um handle retornado é igual a esse valor para saber se a função falhou.

---

### `TH32CS_SNAPMODULE = 0x8`

- **O que é**: Flag para tirar um "snapshot" (foto) de todos os **módulos** (DLLs, EXEs) carregados no processo.
- **Serve para**: Ver quais bibliotecas estão ativas.
- **Usado em**: `CreateToolhelp32Snapshot`.

---

### `TH32CS_SNAPMODULE32 = 0x10`

- **O que é**: Semelhante ao anterior, mas pega apenas módulos de **32 bits**.
- **Importante em**: Sistemas 64-bit, onde processos podem ser mistos (32 e 64 bits).

---

## 📦 Onde isso se aplica?

Essas constantes são base para ferramentas como:
- Injetores de DLL
- Debuggers
- Loaders
- Ferramentas de análise de processos/memória

---

## 🔁 Exemplo prático de uso:

```python
# Abrir o processo com permissões totais
hProcess = OpenProcess(PROCESS_ALL_ACCESS, False, pid)

# Alocar memória remota no processo
remote_addr = VirtualAllocEx(hProcess, NULL, 1024, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)

# Escrever um nome de DLL nessa memória
WriteProcessMemory(hProcess, remote_addr, b"C:\\injeta.dll", len(b"C:\\injeta.dll"), None)

# Criar uma thread remota que roda LoadLibraryA
CreateRemoteThread(hProcess, NULL, 0, LoadLibraryA, remote_addr, 0, NULL)
```

_______________________________________________________________

