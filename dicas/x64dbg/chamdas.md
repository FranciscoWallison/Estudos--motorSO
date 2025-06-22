

# Exempolo de como achamr emquem chamou a função 

Para traçar a chamada que termina em `ret 4` (neste caso no endereço `7709509C`), o objetivo é descobrir:

1. **Quem chamou essa função**
2. **Quais argumentos foram passados (4 bytes = 1 argumento)**
3. **Qual o propósito da função (se possível)**

---

### ✅ Etapas para rastrear no x64dbg (ou outro debugger)

#### 🧭 1. Defina um breakpoint em `7709509C`

No x64dbg:

```
bp 7709509C
```

Ou, se estiver navegando pela UI: clique com o botão direito no endereço e escolha "Set breakpoint on execute".

---

#### 🔍 2. Rode o programa e aguarde o breakpoint

Quando a execução parar em `7709509C`, o ESP estará assim:

* **`ESP` aponta para o endereço de retorno**
* **`ESP+4` é o argumento da função**

Use:

```assembly
dd esp
```

Para ver algo como:

```
ESP     -> 00ABCDEF (endereço de retorno)
ESP+4   -> 00000001 (argumento)
```

---

#### 🔙 3. Identifique o chamador (retorno)

O valor em `[ESP]` (antes do `ret`) é o **endereço para o qual a execução voltará**.

Você pode fazer:

```assembly
d eip  ; mostra bytes da instrução atual
dd esp ; mostra a pilha
```

E depois, siga esse endereço para ver o **código chamador**:

```assembly
disasm [esp]
```

Ou use `Ctrl+G` no x64dbg para navegar diretamente.

---

#### 📦 4. Veja o código da chamada

Você verá algo como:

```assembly
push 00000001
call 77095000  ; ou similar
```

Se sim, bingo. Você encontrou:

* O **argumento**: `0x00000001`
* A **chamada da função**: `call 77095000` → que retorna para `7709509C`
* O **propósito provável** da função depende de onde ela está (por exemplo, dentro da `ntdll.dll` ou alguma DLL personalizada)

---

### 🛠️ Se quiser automatizar com script (opcional)

No x64dbg você pode usar:

```text
bp 7709509C
log "ESP+4 = %x", [esp+4]
```

---

### ❓ Deseja ajuda para identificar **qual é essa função** (nome e descrição)?

Se você souber:

* Qual DLL ou módulo contém `7709509C`
* Ou puder me mostrar o endereço de início da função (ex: `77095000`)

Posso tentar identificar a função por assinatura, pelo nome exportado, ou por análise de padrão.

Se puder, envie:

1. O endereço exato da instrução `call`
2. Alguns bytes anteriores (código que faz o `push`)
3. O nome do módulo (ex: `ntdll.dll`?)

Com isso, posso te ajudar a entender **quem chamou**, **por quê** e **o que faz essa função**.
