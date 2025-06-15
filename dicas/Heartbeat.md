## 🫀 O que é um **Heartbeat** em jogos?

Um **heartbeat** é um tipo de pacote de rede que o **cliente envia periodicamente para o servidor**, com o objetivo de **provar que ainda está conectado e saudável**.

### ✅ Funções principais:

* Mostrar que o jogador ainda está "ativo".
* Impedir que bots/scripts apenas se conectem e fiquem parados.
* Verificar se o cliente é legítimo (ex: hash, processos ativos, flags, etc).
* Sincronizar tempo entre cliente-servidor.

---

## 📦 Exemplo prático de heartbeat:

Imagine um jogo envia a cada 10 segundos:

| Tipo de dado       | Valor        |
| ------------------ | ------------ |
| Packet ID          | `0x0360`     |
| Timestamp          | `1717529991` |
| Hash do executável | `0xA1B2C3D4` |

Esse pacote é algo como:

```hex
0360 E7 2B 5D 66 A1 B2 C3 D4
```

---

## 🔐 Como pode ser protegido?

Jogos podem:

* **Criptografar** os dados.
* Adicionar **tokens aleatórios ou timestamps**.
* Fazer **checagens de integridade** (ex: comparar o hash do `.exe` ou do processo).
* Medir **atraso ou sincronização** (ex: clock drift).
* Fazer heartbeat via WebSocket, UDP ou outra camada.

---

## 🧠 Como pode ser **burlado**?

### 1. **Interceptando com proxy ou sniffer**:

* Usar **Wireshark** ou **mitmproxy** para capturar o heartbeat real.
* Exemplo: capturar o `0x0360` e tentar reproduzi-lo em um bot.

### 2. **Hookando o código de envio**:

* Usar **Frida** ou **x64dbg** para interceptar a função que envia o heartbeat.
* Assim, você pode alterar o valor enviado, pausá-lo ou automatizá-lo.

```python
# Exemplo: hook com Frida
Interceptor.attach(Module.findExportByName(null, "send"), {
    onEnter(args) {
        if (Memory.readU16(args[0]) == 0x0360) {
            console.log("Heartbeat interceptado!");
            // Modifica conteúdo
        }
    }
});
```

### 3. **Criando um fake client / bot**:

* Reproduzir só os pacotes de login e heartbeat.
* Mas é preciso **emular os valores corretos**, ou o servidor detecta inconsistência.

### 4. **Patch no cliente**:

* Usar **Ghidra** ou **IDA** para localizar o trecho que envia heartbeat.
* Patchar com `RET` (0xC3) ou `NOPs` para parar o envio.

---

## 🚨 Como o servidor detecta burlas?

* Verifica **tempo entre heartbeats**.
* Detecta **dados inválidos ou repetidos**.
* Desconecta se não receber heartbeat dentro de X segundos.
* Usa o heartbeat para **validar o cliente** (hash, versão, injeções, etc).

---

## Ferramentas úteis para análise:

| Nome           | Função                                         |
| -------------- | ---------------------------------------------- |
| **Wireshark**  | Captura e analisa pacotes                      |
| **Frida**      | Intercepta e modifica funções do cliente       |
| **Ghidra/IDA** | Análise estática para localizar heartbeat      |
| **x64dbg**     | Debugger para ver quando o heartbeat é enviado |
| **Scapy**      | Scriptar envio de pacotes personalizados       |

---

Dicas:

* Escrever um script para simular heartbeat,
* Mostrar como detectar onde o heartbeat é construído no binário com Ghidra,
* Ou criar um patch simples com Frida ou x64dbg para bloquear/enganar o envio.
