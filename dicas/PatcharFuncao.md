“**Patchar função**” (ou *function patching* em inglês) é um termo comum nas áreas de programação, hacking, engenharia reversa e desenvolvimento de software. Basicamente, significa **modificar o comportamento de uma função** (ou método) de um programa, geralmente em tempo de execução, para que ela faça algo diferente do que foi originalmente planejado.

### Como funciona na prática?

* **No código fonte**: Patchar uma função pode ser simplesmente trocar a implementação de uma função em um código aberto.
* **No binário**: Em programas já compilados, patchar significa alterar diretamente o código de máquina de uma função — por exemplo, usando um editor hexadecimal, um disassembler (como Ghidra ou IDA Pro) ou técnicas de injeção de código/dll.

### Exemplos práticos

1. **Trocar um retorno**
   Se você tem uma função `int checarLicenca()`, que retorna `0` para falso e `1` para verdadeiro, patchar pode ser simplesmente forçar o retorno para sempre ser `1` — ou seja, sempre aceitar qualquer licença.
2. **Desativar uma verificação**
   Patchar pode ser usado para **remover** proteções, verificações de integridade, ou trechos de código indesejados (como DRM, anti-cheat, etc).
3. **Hook/Interceptação**
   Em desenvolvimento de cheats ou automações, patchar uma função pode significar redirecionar o fluxo de execução: toda vez que a função original é chamada, o controle vai para uma função customizada (“hook”).

### Métodos comuns de patch

* **Patch binário**: Alterar bytes específicos do executável.
* **Hook em runtime**: Usar bibliotecas ou técnicas para trocar o ponteiro da função por outro (ex: Detours, MinHook, vtables em C++).
* **DLL Injection**: Injetar uma DLL que sobrescreve funções na memória do processo.

### Exemplo simples (pseudo-código):

```c
// Função original
int checarLicenca() {
    // lógica complicada
    return 0; // licença inválida
}

// Após patch
int checarLicenca() {
    return 1; // sempre válido
}
```

No binário, às vezes o patch é feito mudando instruções tipo:

```
JNZ SHORT 00401020  ; Salta se não for zero (licença inválida)
```

Para:

```
JMP SHORT 00401020  ; Sempre pula (licença válida)
```

