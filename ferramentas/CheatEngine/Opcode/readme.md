

# **Instruções de Assembly (Opcodes)**

## **Operações Aritméticas**
- **`dec`** – Diminui o valor em 1.  
- **`inc`** – Aumenta o valor em 1.  
- **`sub`** – Subtrai o segundo operando do primeiro.  
  - Exemplo: `sub [ebx+00000310],4` → Diminui o valor em `[ebx+0310]` por 4.  
- **`add`** – Soma o segundo operando ao primeiro.  
  - Exemplo: `add [ebx+00000310],4` → Aumenta o valor em `[ebx+0310]` por 4.  

## **Movimentação e Cálculo de Endereços**
- **`mov`** – Copia o segundo operando para o primeiro.  
  - Exemplo: `mov [ebx+00000310],4` → Altera o valor em `[ebx+0310]` para 4.  
- **`lea`** – Calcula e copia um endereço para o registrador.  
  - Exemplo: `lea eax,[esi+30]` → Copia `esi+30` para `eax`.  
  - Útil para economizar operações com ponteiros.

## **Comparação e Saltos**
- **`cmp`** – Compara dois valores (registradores ou registrador e número).  
  - Exemplo:  
    ```assembly
    cmp esi,2     ; Compara `esi` com 2
    cmp esi,ecx   ; Compara `esi` com `ecx`
    ```
  - O resultado afeta os sinalizadores (flags).

- **`jmp`** – Salta para um endereço específico ou relativo.  
  - Exemplo: `jmp +0000000A` → Salta 10 bytes à frente no código.

### **Saltos Condicionais**
- **`je`** – Salta se a comparação anterior for igual.  
  ```assembly
  cmp esi,2
  je 0f445566  ; Se `esi` for 2, pula para o endereço `0f445566`.
  ```
- **`jne`** – Salta se a comparação não for igual.  
- **`jg`** – Salta se o primeiro operando for maior.  
- **`jl`** – Salta se o primeiro operando for menor.  

## **Pilha e Manipulação de Registradores**
A pilha é uma estrutura **LIFO (Last In, First Out)**, ou seja, o último valor adicionado é o primeiro removido.

### **Comandos da Pilha**
- **`push`** – Salva um valor na pilha.  
- **`pop`** – Remove um valor da pilha e o coloca em um registrador.  
- **`pushad` / `popad`** – Salva/carrega todos os registradores gerais.  
- **`pushfd` / `popfd`** – Salva/carrega todos os sinalizadores (flags).  

### **Exemplo de Uso da Pilha**
Suponha que temos:  
```
Pilha inicial:
4
5
6
```
Executando:
```assembly
push ecx   ; Salva o valor de `ecx` na pilha
push edx   ; Salva `edx` na pilha
```
A pilha agora:
```
2  (edx)
3  (ecx)
4
5
6
```
Agora, restaurando os valores:
```assembly
pop ecx   ; `ecx` recebe 2
pop edx   ; `edx` recebe 3
```
Resultado:
```
4
5
6
```
Isso significa que trocamos os valores entre `ecx` e `edx`.

### **Ordem Correta de Uso**
Ao usar `pushad` e `pushfd`, é fundamental respeitar a ordem correta:
```assembly
pushad
pushfd
...
popfd
popad
```
Ou:
```assembly
pushfd
pushad
...
popad
popfd
```
Se a ordem for errada, os valores serão restaurados incorretamente, potencialmente causando falhas no programa.

---

# **Aplicações Práticas**
## **1. Criando um "Modo Deus" em Warhammer: Mark of Chaos**
Objetivo: Criar um script para tornar apenas suas unidades invencíveis, sem afetar os inimigos.

### **Passo 1: Encontrar a variável de saúde**
Ao monitorar a memória, encontramos:
```assembly
008d0f08 - d9 56 04  ; fst dword ptr [esi+04]
```
A estrutura de dados do soldado:
```
[esi]     = ID do jogador (0 = jogador, ≠0 = inimigo)
[esi+04]  = Saúde atual
[esi+08]  = Saúde máxima
```

### **Passo 2: Criando o Script**
O script precisa verificar se a unidade pertence ao jogador e, caso positivo, definir a saúde igual ao valor máximo:

```assembly
fst dword ptr [esi+04]  ; Código original que altera a saúde
pushad                  ; Salva os registradores
pushfd                  ; Salva os sinalizadores

cmp [esi],0             ; Verifica se a unidade pertence ao jogador
jne +6                  ; Se for inimigo, pula as próximas instruções
mov eax,[esi+08]        ; Copia a saúde máxima para `eax`
mov [esi+04],eax        ; Define a saúde igual à máxima

popfd                   ; Restaura os sinalizadores
popad                   ; Restaura os registradores
```
**Resultado:** Suas unidades serão invencíveis, enquanto os inimigos ainda sofrerão danos.

---

## **2. Dinheiro Ilimitado no C&C Generals**
### **Passo 1: Encontrar o Código que Lê o Dinheiro**
Ao analisar a memória, encontramos dois endereços:
1. **Valor real do dinheiro.**
2. **Valor exibido na tela.**

O jogo verifica o saldo constantemente e exibe o valor atualizado.

### **Passo 2: Identificar a Instrução de Leitura**
```assembly
mov ebx,[eax+38]  ; Lê o dinheiro do jogador
```
### **Passo 3: Criando o Script**
Podemos interceptar essa leitura e modificar o valor:

```assembly
mov ebx,[eax+38]       ; Lê o dinheiro real
mov [eax+38],000f423f  ; Define dinheiro para 999999 (0x000f423f em hex)
```

### **Por que Funciona?**
- Esse código **só afeta o dinheiro do jogador**, pois a exibição na tela refere-se apenas ao jogador.
- O inimigo continua com seu dinheiro normal.
