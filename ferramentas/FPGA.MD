### **🔹 FPGA (Field-Programmable Gate Array)**
📌 **O que é?**  
FPGA é um **chip programável** que pode ser configurado após a fabricação para executar tarefas específicas de hardware. Ele é diferente de um processador comum (CPU/GPU) porque permite criar circuitos personalizados para diversas funções.

📌 **Por que é usado em Game Hacking?**  
- FPGAs podem ser usados para **desviar sistemas anti-cheat** porque operam **fora do sistema operacional**, evitando detecção.
- São usados em **ataques de leitura/escrita direta na memória (DMA)** para manipular valores em tempo real.
- Aplicações em **hardware-level cheats**, como **aimbots indetectáveis** que capturam dados da tela e processam comandos.

📌 **Exemplo:**  
- Alguns hackers usam **FPGAs conectados via PCIe** para **ler a memória do jogo sem que o anti-cheat perceba**.

---

### **🔹 DMA (Direct Memory Access)**
📌 **O que é?**  
DMA é um **método de acesso direto à memória** do sistema sem passar pela CPU. Ele é usado por dispositivos de hardware para transferir dados rapidamente sem sobrecarregar o processador.

📌 **Por que é usado em Game Hacking?**  
- **Permite ler e modificar a memória do jogo diretamente**, sem que o sistema operacional ou o anti-cheat percebam.
- Como **não requer acesso de software**, é muito mais difícil de detectar.
- **Cheats baseados em DMA** podem rodar em um **PC externo** e acessar o jogo via PCIe, tornando-os praticamente indetectáveis.

📌 **Exemplo:**  
- Um hacker pode usar um **cartão FPGA com acesso DMA** para ler a memória do jogo e **criar um ESP (wallhack) externo** sem ser detectado.

---

### **🔹 PCILeech & PCILeech-WebRadar**
📌 **O que é PCILeech?**  
PCILeech é uma ferramenta que explora **acesso DMA via PCIe** para ler e modificar a memória RAM do computador.  
Ele pode ser usado para **despejar memória, injetar código ou até mesmo contornar proteções do sistema**.

📌 **O que é PCILeech-WebRadar?**  
PCILeech-WebRadar é uma **extensão da ferramenta PCILeech**, projetada para fornecer uma **interface web para visualizar e modificar a memória** do sistema-alvo.

📌 **Por que é usado em Game Hacking?**  
- **Pode ser usado para acessar a memória do jogo sem ser detectado.**
- **Evita software anti-cheat**, pois a leitura da memória ocorre via hardware (PCIe) em vez de software tradicional.
- **Criação de cheats remotos**: Como o ataque ocorre via hardware, ele pode ser operado por outro PC conectado ao alvo.

📌 **Exemplo:**  
- Um hacker pode usar **PCILeech conectado a um FPGA via PCIe** para **ler a memória do jogo, encontrar valores importantes e manipular o jogo em tempo real** sem ser detectado.

---

### **📌 Resumo e Relação entre os Conceitos**
| Tecnologia  | O que é? | Uso em Game Hacking |
|------------|---------|----------------|
| **FPGA**  | Chip programável para tarefas específicas | Usado para cheats via hardware, como leitura de memória e aimbots |
| **DMA**  | Acesso direto à memória RAM sem CPU | Permite cheats indetectáveis lendo e modificando valores diretamente |
| **PCILeech** | Ferramenta para acessar memória via PCIe | Pode ser usada para leitura e manipulação da memória de jogos sem ser detectado |

---

### **🔹 Como isso impacta os jogos online?**
- Cheats **baseados em FPGA e DMA são extremamente difíceis de detectar** porque não deixam rastros no software.
- Anti-cheats como **Vanguard (Valorant)** e **BattlEye (PUBG, Rainbow Six)** **tentam bloquear acesso PCIe não autorizado**, mas novas técnicas continuam surgindo.
- Desenvolvedores estão explorando **métodos de detecção baseados em comportamento** (machine learning), já que detectar fisicamente esses hacks é quase impossível.

---

### **Como poderia ser usado FPGA + DMA + PCILeech ao Mesmo Tempo para Game Hacking**
Se combinarmos **FPGA, DMA e PCILeech**, podemos criar um **hack indetectável e extremamente poderoso**. Isso porque:
1. **FPGA** pode atuar como um controlador personalizado para acessar a memória do jogo.
2. **DMA** permite **ler e modificar a memória** do jogo diretamente via **PCIe**, sem passar pelo sistema operacional.
3. **PCILeech** fornece um software para **visualizar, analisar e modificar** a memória do jogo remotamente.

---

## **🔹 Estrutura do Hack Utilizando FPGA, DMA e PCILeech**
💡 **O plano é criar um cheat externo usando outro computador** para manipular o jogo de maneira invisível para o anti-cheat.

### **📌 Requisitos de Hardware & Software**
🔧 **Hardware Necessário:**
1. **Placa FPGA com acesso DMA via PCIe**
   - Modelos comuns: **Screamer PCIe**, **Cortex DMA**, **LeechCore FPGA**
2. **Dois computadores**:
   - **PC 1 (Máquina do Jogo)** → Rodando o jogo (exemplo: Valorant, Apex, CS2).
   - **PC 2 (Máquina Hacker)** → Monitorando e manipulando a memória via PCIe.

💻 **Software Utilizado:**
1. **PCILeech** (Ferramenta para acessar a memória do jogo via PCIe).
2. **FPGA com firmware DMA** (Para ler/escrever a memória do PC 1).
3. **Script customizado** (Python/C++ para processar os dados e aplicar cheats).

---

## **🔹 Etapas para Criar o Hack**
### **1️⃣ Instalar PCILeech no PC 2 (Máquina Hacker)**
- No **PC 2**, instale **PCILeech** para monitorar e modificar a memória do jogo no **PC 1**.
- Conecte a **placa FPGA ao PC 1 via PCIe**.

```bash
git clone https://github.com/ufrisk/pcileech.git
cd pcileech
```

---

### **2️⃣ Configurar FPGA para Ler/Escrever Memória via DMA**
- Carregue o firmware correto no **FPGA** para permitir acesso **DMA**.
- Use um comando como este para capturar um dump da memória do jogo:

```bash
pcileech.exe dump -device fpga -out memory_dump.bin
```

Agora, temos acesso **direto** à memória do jogo!

---

### **3️⃣ Encontrar os Endereços de Memória do Jogo**
Usamos **PCILeech** para encontrar valores como:
- **Vida do jogador**
- **Dinheiro**
- **Posição dos inimigos (para ESP)**
- **Coordenadas do jogador**

```bash
pcileech.exe scan -device fpga -mem 0x100000000
```

Ou, se soubermos um valor específico (por exemplo, dinheiro = 5000):

```bash
pcileech.exe find -val 5000
```

---

### **4️⃣ Criar um Script para Modificar o Dinheiro**
Agora que encontramos o endereço do dinheiro, podemos modificá-lo **sem que o anti-cheat perceba**:

```python
import pcileech

device = pcileech.open_device("fpga")

# Endereço onde o dinheiro do jogador está armazenado
MONEY_ADDRESS = 0x12345678

# Ler valor atual
current_money = pcileech.read_memory(device, MONEY_ADDRESS, 4)
print(f"Dinheiro atual: {current_money}")

# Adicionar mais dinheiro
new_money = current_money + 10000
pcileech.write_memory(device, MONEY_ADDRESS, new_money, 4)

print(f"Novo dinheiro: {new_money}")
```

**🎯 Agora, toda vez que o jogo atualizar o dinheiro, ele será aumentado automaticamente!**

---

### **5️⃣ Criar um Aimbot/ESP usando DMA**
Se quisermos usar **ESP (Wallhack) ou Aimbot**, podemos:
1. **Ler a posição dos inimigos**.
2. **Alterar nossa mira automaticamente**.

```python
ENEMY_POS = 0x1234ABCD  # Endereço da posição dos inimigos
MY_AIM = 0x5678DCBA  # Endereço da mira do jogador

# Pega a posição do inimigo
enemy_x, enemy_y, enemy_z = pcileech.read_memory(device, ENEMY_POS, 12)

# Atualiza nossa mira para apontar para o inimigo
pcileech.write_memory(device, MY_AIM, enemy_x, 4)
pcileech.write_memory(device, MY_AIM + 4, enemy_y, 4)
```

✅ Agora, toda vez que o jogo atualizar a posição do inimigo, nossa mira será ajustada **automaticamente** para mirar nele!

---

## **🔹 Como Isso Evita a Detecção do Anti-Cheat?**
🚀 **FPGA + DMA + PCILeech tornam esse hack quase indetectável** porque:
1. **Nada é modificado na memória do sistema operacional** (todos os acessos são feitos via **PCIe**).
2. **Não há injeção de código** (`DLL Injection`), que é o que a maioria dos anti-cheats detecta.
3. **Rodamos o hack em um segundo PC**, tornando mais difícil rastrear atividades suspeitas.

---

## **📌 Resumo Final**
| Tecnologia  | Função | Como Contribui para o Hack? |
|------------|--------|-----------------------------|
| **FPGA**  | Hardware reprogramável | Permite acesso DMA à memória do jogo |
| **DMA**  | Acesso direto à memória | Lê e altera valores sem passar pelo sistema operacional |
| **PCILeech** | Ferramenta de análise de memória | Captura dados do jogo e injeta modificações |

✅ **Isso permite criar hacks como**:
- **Aimbot** (ajustando a mira automaticamente via DMA).
- **ESP (Wallhack)** (exibir inimigos no mapa).
- **Infinite Money / HP** (modificando valores de memória diretamente).