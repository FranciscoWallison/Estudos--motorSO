A **PsTools** é uma suíte de utilitários de linha de comando desenvolvida pela equipe **Sysinternals da Microsoft**. Ela é extremamente popular entre administradores de sistemas e usuários avançados do Windows por sua capacidade de gerenciar e solucionar problemas em sistemas Windows, tanto localmente quanto, e mais importante, **remotamente**.

---

### O que torna a PsTools tão poderosa?

O grande diferencial da PsTools é sua capacidade de executar tarefas administrativas em computadores remotos **sem a necessidade de instalar software cliente** neles. Isso a torna uma ferramenta inestimável para gerenciar múltiplas máquinas em uma rede. A suíte é composta por diversas ferramentas individuais, cada uma com sua funcionalidade específica.

---

### Principais Ferramentas da Suíte PsTools

A suíte PsTools inclui uma variedade de utilitários, sendo os mais notáveis:

* **PsExec:** Provavelmente a ferramenta mais conhecida e poderosa da suíte. Ela permite que você execute processos em sistemas remotos com os privilégios que você especificar. É como ter um "Telnet" aprimorado, permitindo rodar comandos e programas interativamente em outra máquina sem precisar de uma conexão de desktop remoto completa.
    * **Exemplos de uso:**
        * Abrir um prompt de comando interativo em um PC remoto: `psexec \\computador_remoto cmd`
        * Executar um script ou programa em um PC remoto: `psexec \\computador_remoto C:\caminho\para\programa.exe`
        * Reiniciar um serviço em um PC remoto: `psexec \\computador_remoto net stop "Nome do Serviço"` e `psexec \\computador_remoto net start "Nome do Serviço"`
* **PsInfo:** Exibe informações detalhadas sobre um sistema, como versão do sistema operacional, uptime (tempo de atividade), informações de hardware, patches instalados, etc. Útil para auditoria e inventário.
* **PsList:** Lista informações detalhadas sobre os processos em execução em um sistema, similar ao Gerenciador de Tarefas, mas com mais detalhes e a capacidade de listar processos em máquinas remotas.
* **PsKill:** Permite encerrar processos em um sistema local ou remoto, usando o nome do processo ou o ID do processo (PID).
* **PsService:** Permite exibir, iniciar, parar ou configurar serviços em sistemas locais ou remotos.
* **PsShutdown:** Desliga ou reinicia um computador local ou remoto, com diversas opções, como agendamento e mensagens para o usuário.
* **PsLoggedOn:** Mostra quais usuários estão logados em um sistema (localmente e via compartilhamento de recursos).
* **PsGetSid:** Exibe o SID (Security Identifier) de um computador ou de um usuário.
* **PsFile:** Mostra os arquivos que estão abertos remotamente em um sistema.
* **PsLogList:** Permite despejar registros de log de eventos de um sistema.
* **PsPasswd:** Permite alterar senhas de contas.
* **PsPing:** Uma ferramenta para medir o desempenho da rede, similar ao `ping` e `tracert`, mas com funcionalidades aprimoradas para testar latência e largura de banda.
* **PsSuspend:** Suspende processos, o que pode ser útil para depurar ou gerenciar o uso de recursos.

---

### Como usar a PsTools

1.  **Download:** Baixe o pacote PsTools diretamente do site oficial do Sysinternals da Microsoft. Ele vem como um arquivo `.zip`.
2.  **Extração:** Extraia o conteúdo do arquivo `.zip` para uma pasta de sua preferência (ex: `C:\PsTools`).
3.  **Adicionar ao PATH (Opcional, mas recomendado):** Para facilitar o uso, adicione o caminho da pasta onde você extraiu a PsTools à variável de ambiente `PATH` do seu sistema. Isso permite que você execute qualquer ferramenta da PsTools de qualquer diretório no prompt de comando ou PowerShell.
    * Para fazer isso: Pesquise por "Variáveis de Ambiente" no Windows, clique em "Variáveis de Ambiente...", na seção "Variáveis do Sistema", encontre a variável `Path`, clique em "Editar", e adicione o caminho da sua pasta `PsTools`.
4.  **Execução:** Abra um **Prompt de Comando (CMD) ou PowerShell como Administrador**. Navegue até a pasta da PsTools (se não adicionou ao PATH) ou simplesmente digite o nome da ferramenta (ex: `psexec`).
5.  **Sintaxe Básica:** A maioria das ferramentas PsTools segue uma sintaxe comum:
    `[NomeDaFerramenta] \\computador_remoto [opções] [comando_a_executar]`
    * **`\\computador_remoto`**: Substitua pelo nome do computador remoto ou seu endereço IP. Você pode usar `\\*` para todos os computadores no domínio atual, ou `@arquivo` para especificar um arquivo de texto com uma lista de computadores.
    * **`-u username -p password`**: Para especificar credenciais de um usuário com permissões administrativas no computador remoto. Se você não fornecer a senha (`-p`), a ferramenta a solicitará.
    * **`-s`**: Para executar o processo no contexto da conta de sistema local no computador remoto.
    * **`-i`**: Para permitir que o processo interaja com a área de trabalho do usuário.

---

### Considerações Importantes

* **Permissões:** Para usar a PsTools em computadores remotos, você precisará de credenciais de administrador (ou equivalentes) no sistema de destino.
* **Firewall:** O firewall do Windows nos computadores de destino deve permitir as conexões necessárias para que a PsTools funcione corretamente (geralmente via portas de compartilhamento de arquivos e impressoras, como a porta TCP/445).
* **Antivírus:** Alguns softwares antivírus podem sinalizar as ferramentas PsTools como "ameaças" (especialmente o PsExec) porque elas podem ser usadas por malwares para controle remoto. No entanto, as ferramentas Sysinternals são legítimas e seguras quando baixadas da fonte oficial da Microsoft.
* **EULA:** Na primeira vez que você executa uma ferramenta PsTools, ela geralmente exibirá um Contrato de Licença de Usuário Final (EULA) que você precisa aceitar.

---

A PsTools é uma caixa de ferramentas indispensável para qualquer profissional de TI que lida com ambientes Windows, permitindo automação, diagnóstico e gerenciamento remoto de forma eficiente.



---

A PsTools, em particular o **PsExec**, permite que você execute comandos em um nível de privilégio muito alto no Windows, que é a conta **SYSTEM (NT AUTHORITY\\SYSTEM)**. Embora não seja "nível de kernel" no sentido estrito (ou seja, não é o mesmo que um driver de kernel ou código executando diretamente no kernel), para a maioria das tarefas administrativas e de solução de problemas, ter acesso como a conta SYSTEM é o mais próximo que você pode chegar de um controle total sobre o sistema no espaço do usuário.

-----

### Entendendo a conta SYSTEM

A conta **NT AUTHORITY\\SYSTEM** é uma conta de serviço interna usada pelo sistema operacional Windows e por vários serviços do Windows. Ela possui privilégios muito extensos na máquina local, mais do que a conta de administrador padrão. Muitos processos críticos do sistema são executados sob esta conta.

  * **Não é o kernel:** É importante diferenciar a conta SYSTEM do "kernel" do sistema operacional. O kernel é a parte central do SO que gerencia recursos de hardware e software e fornece serviços essenciais. Código de kernel executa em um modo de privilégio de CPU diferente (Ring 0 na arquitetura x86/x64) e tem acesso direto ao hardware. A conta SYSTEM, por outro lado, é uma conta de usuário (mesmo que seja uma conta de sistema especial) que executa no modo de usuário (Ring 3), mas com privilégios de segurança muito elevados.
  * **Privilégios elevados:** Embora não seja o kernel, ter um prompt de comando ou executar um processo como SYSTEM permite que você faça quase qualquer coisa na máquina local que um usuário faria, ignorando muitas restrições de permissão que até mesmo um administrador normal enfrentaria. Isso é incrivelmente útil para:
      * Acessar e modificar arquivos e chaves de Registro protegidos.
      * Reiniciar ou interromper serviços que normalmente não permitiriam.
      * Instalar e desinstalar software que requer privilégios elevados.
      * Diagnosticar problemas de permissão.

-----

### Como usar o PsExec para obter um prompt de comando como SYSTEM

O comando que você procura para obter um prompt de comando (CMD) ou PowerShell como a conta SYSTEM é com o **PsExec**:

```bash
psexec -i -s cmd.exe
```

Vamos quebrar os parâmetros:

  * **`-i` (Interactive):** Este parâmetro faz com que o processo seja executado interativamente, o que significa que ele aparecerá em sua sessão de usuário e você poderá interagir com ele (ou seja, ver e digitar no prompt de comando). Sem este parâmetro, o processo pode ser executado em uma sessão não interativa e você não o veria.
  * **`-s` (System):** Este é o parâmetro crucial. Ele instrui o PsExec a executar o processo especificado (neste caso, `cmd.exe`) sob o contexto da conta **NT AUTHORITY\\SYSTEM**.
  * **`cmd.exe`:** É o executável do prompt de comando que você deseja abrir com esses privilégios elevados. Você pode substituí-lo por `powershell.exe` para obter uma sessão do PowerShell como SYSTEM.

-----

#### Passos para executar:

1.  **Baixe e extraia o PsTools:** Certifique-se de ter o `PsExec.exe` (que faz parte da suíte PsTools) em um local acessível, de preferência na variável de ambiente PATH do seu sistema, ou navegue até a pasta onde ele foi extraído.
2.  **Abra o Prompt de Comando (ou PowerShell) como Administrador:** Você precisará de privilégios de administrador para executar o PsExec com sucesso e iniciar processos com a conta SYSTEM.
3.  **Execute o comando:** No prompt de comando elevado, digite:
    ```bash
    psexec -i -s cmd.exe
    ```
4.  **Verifique a conta:** Uma nova janela do prompt de comando será aberta. Para confirmar que você está na conta SYSTEM, digite `whoami` nesta nova janela. A saída deve ser `nt authority\system`.

-----

### Casos de uso comuns para um prompt SYSTEM:

  * **Remover arquivos teimosos:** Arquivos que não podem ser excluídos mesmo como administrador podem ser removidos com um prompt SYSTEM.
  * **Modificar permissões:** Alterar permissões em arquivos ou pastas que estão bloqueados.
  * **Diagnóstico de serviços:** Testar o comportamento de aplicativos ou serviços que precisam interagir com componentes de baixo nível do sistema.
  * **Instalação/remoção de software problemático:** Alguns instaladores ou desinstaladores podem falhar devido a permissões; um prompt SYSTEM pode contornar isso.
  * **Reativar contas desativadas:** Às vezes, é a única maneira de acessar certas configurações ou reativar contas de usuário.

-----

Lembre-se que usar a conta SYSTEM concede um controle imenso sobre o sistema, então use com cautela e apenas quando souber o que está fazendo, pois ações incorretas podem causar instabilidade no sistema.