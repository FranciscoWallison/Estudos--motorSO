**EAC** (Easy Anti-Cheat) é um sistema anti-cheat usado por muitos jogos (tipo *Fortnite*, *Apex Legends*, *Rust*...) para detectar, bloquear e dificultar trapaças.
A lógica principal dele é baseada em **camadas de proteção**, combinando **detecção ativa** e **prevenção proativa**.

Vou te resumir a **estrutura lógica do EAC**:

---

### 1. **Driver no Kernel Mode** (Ring 0)

* O EAC instala **um driver no kernel** do Windows (normalmente assinado pela Epic Games).
* Esse driver:

  * **Monitora o sistema** procurando processos e módulos suspeitos.
  * **Detecta injeções** de DLL em jogos.
  * **Verifica hooks** de APIs críticas.
  * **Protege o próprio processo do jogo** contra manipulações externas (ex: *OpenProcess*, *WriteProcessMemory*, *ReadProcessMemory*).
  * **Bloqueia debuggers** (como x64dbg, Cheat Engine).
  * Às vezes faz **scan direto na memória física** (RAM) para encontrar alterações que o próprio Windows esconderia.

---

### 2. **Client-side Service** (Serviço de usuário)

* Existe um serviço no *User Mode* rodando junto com o jogo.
* Esse serviço:

  * Faz **checks de integridade** nos arquivos do jogo (hash dos executáveis, DLLs...).
  * **Valida o ambiente** (detecta se o jogo está sendo rodado em ambiente virtual, injetores, editores de memória, etc.).
  * **Comunica constantemente** com servidores da EAC.
  * Reporta tentativas de manipulação ao servidor para banimento automático ou análise posterior.

---

### 3. **Server-side Validation**

* No servidor (fora da máquina do jogador):

  * O EAC cruza dados enviados pelo cliente.
  * Valida inconsistências como:

    * Movimentação absurda (ex: *speed hack*, *teleport*).
    * Alterações em variáveis do jogo (*wallhack*, *aimbot*, etc.).
    * Comportamento impossível com base nas físicas do jogo.

---

### 4. **Técnicas de Obfuscação e Proteção**

* O EAC se atualiza dinamicamente: novos módulos podem ser baixados toda vez que o jogo inicia.
* Eles usam:

  * **Obfuscação pesada** no código (para retardar engenharia reversa).
  * **Detection de debuggers escondidos** (inclusive bypassados).
  * **Checks redundantes** (uma proteção confere a outra).
  * **Anti-hook**: verifica se funções do sistema (ex: `NtReadVirtualMemory`, `Send`, `Recv`, etc.) estão hookadas.
  * **TLS Callbacks** escondidos: inicializam verificações antes mesmo do *main* do jogo começar.
  * Às vezes, utilizam **custom syscall**: invocam funções do Windows sem passar pelo usermode "normal", burlando hooks comuns.

---

### 5. **Detectar ou Bloquear Cheat Engines Comuns**

* O EAC já conhece ferramentas de cheat como Cheat Engine, Extreme Injector, x64dbg, Process Hacker, etc.
* Se alguma dessas ferramentas é detectada:

  * O jogo pode fechar automaticamente.
  * Ou o jogador pode ser banido automaticamente ou manualmente depois de análise.

---

### Em resumo:

> **O EAC é uma combinação de:**
> 🔹 *Driver kernel* + *Serviço no usuário* + *Verificação de servidor* + *Atualizações dinâmicas* + *Anti-tamper e anti-reversão*.

É por isso que burlar o EAC exige geralmente:

* **Drivers próprios** (spoofers, mappers).
* **Kernel-level cheats** (Ring 0).
* **Bypasses de assinatura de driver** (ex: PatchGuard, DSE desabilitado).
* Ou **Exploits mais sofisticados** que passem despercebidos em memória.

---

**exemplos práticos** de técnicas que o EAC tenta bloquear (como *manual mapping*, *thread hijacking*, *direct syscalls*) ou até uma visão de como funcionaria um **bypass básico** em teoria (por exemplo, usando um "clean driver" e "usermode hide").
