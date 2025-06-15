
* **Compilador KMDF 10.0 “Universal”** usado ✔
* `StartSuspended.sys` gerado em **x64\Release** ✔
* Assinatura de teste aplicada ao `.sys` e ao catalog `.cat` ✔
* **Signability test**: “Errors: None / Warnings: None” ✔
* Build “1 bem-sucedida, 0 com falha” ✔

Ou seja, o binário está pronto para ser carregado em modo **test-signing**.

### Próximos passos rápidos

1. **Copie o driver** (e o `.cat`, se for instalar via INF) para um diretório estável, ex. `C:\Drivers`.
2. Certifique-se de que o sistema está em test-signing e, se a VM tiver Secure Boot, que ele esteja desativado.

   ```cmd
   bcdedit /set testsigning on
   ```

   > Reinicie após alterar. E veja se aparece o modo teste no cando direito da tela.
3. **Crie o serviço kernel** (substitua o caminho conforme sua cópia):

   ```cmd
   sc create StartSuspended type= kernel ^
            binPath= "C:\Drivers\StartSuspended.sys"
   ```
Resultado: `[SC] CreateService ÊXITO`

4. **Configure o alvo** no registro
   `HKLM\SYSTEM\CurrentControlSet\Services\StartSuspended` →
   valor **Target** (REG\_SZ) = `Game.exe` (ou o executável que quiser congelar).
   
   ### Configurando o **Target** direto no terminal (sem abrir o Regedit)

> Execute o terminal **como Administrador** — o hive `HKLM` exige privilégio elevado.

#### 1) Usando **CMD** (`game.exe`)

```cmd
rem Cria/atualiza o valor “Target” como REG_SZ
reg add "HKLM\SYSTEM\CurrentControlSet\Services\StartSuspended" ^
    /v Target /t REG_SZ /d Game.exe /f
```
Resultado: `A operação foi concluída com êxito.`

* `/v Target` → nome do valor
* `/t REG_SZ` → tipo string
* `/d Game.exe` → dado gravado (substitua pelo executável desejado)
* `/f`     → força sobrescrita se já existir

Verifique:

```cmd
reg query "HKLM\SYSTEM\CurrentControlSet\Services\StartSuspended" /v Target
```

 posicionado na chave	regedit →
quando o Editor abrir, cole HKLM\SYSTEM\CurrentControlSet\Services\StartSuspended na barra de endereços (ela aceita colar/Enter)

---

**Quando fazer?**
Grave o valor **antes** de rodar `sc start StartSuspended`; o driver lê o registro apenas na inicialização.

**Dica:** não precisa informar caminho completo — o driver compara apenas o **nome do executável** (case-insensitive).

5. **Inicie o driver**:

   ```cmd
   sc start StartSuspended
   ```

   – O próximo processo cujo nome bater com “Target” já nascerá suspenso.
6. Retome o processo com o `ResumeProcess.exe` fornecido ou pelo Resource Monitor.
7. Quando terminar:

   ```cmd
   sc stop StartSuspended
   sc delete StartSuspended
   bcdedit /set testsigning off    &rem (opcional, se quiser voltar a modo normal)
   ```

Se precisar depurar/pesquisar logs, ative o **DebugView** (Kernel Capture + Verbose) antes de dar `sc start…` — o driver usa `KdPrintEx`.

Parabéns: a etapa de build foi concluída com sucesso; basta seguir o fluxo de instalação e teste.

### Resumo do Fluxo de Trabalho de Desenvolvimento

Para evitar esse erro no futuro, sempre que você compilar uma nova versão do seu driver e precisar reinstalá-lo, siga este ciclo:

1.  **Pare** o serviço antigo: `sc stop [nome_do_servico]`
2.  **Delete** o serviço antigo: `sc delete [nome_do_servico]`
3.  (Copie seu novo arquivo `.sys` para a pasta de destino)
4.  **Crie** o novo serviço: `sc create [nome_do_servico] ...`
5.  **Inicie** o novo serviço: `sc start [nome_do_servico]`
