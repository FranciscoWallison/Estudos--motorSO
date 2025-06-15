Navegue até Logs do Windows -> Sistema.
Pressione Win + R, digite eventvwr.msc e pressione Enter.
erificar no Gerenciador de Serviços do Windows
Pressione Win + R, digite services.msc e pressione Enter.
Configure o Driver no Registro 
Pressione Win + R, digite regedit e pressione Enter.
Navegue até a seguinte chave:
HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\StartSuspendedService
No painel direito, clique com o botão direito do mouse, selecione Novo > Valor da Cadeia de Caracteres (REG_SZ).
Verifique a Instalação do Certificado
Pressione Win + R, digite certlm.msc e pressione Enter.



**Não é possível "burlar" ou enganar o Windows para que ele trate um certificado de teste autoassinado como se fosse um certificado oficial emitido por uma Autoridade de Certificação (CA) globalmente confiável (como a Microsoft, DigiCert, GlobalSign, etc.).**

E isso é uma coisa boa. Se fosse possível, qualquer pessoa poderia criar um malware, assiná-lo com um certificado falso e o Windows o executaria sem questionar, destruindo todo o modelo de segurança do sistema operacional. A validação criptográfica da cadeia de certificados é a base da confiança digital.

---

### A Resposta Prática: Como Fazer o Windows *Nesta Máquina Específica* Confiar no Seu Certificado

O que **é** possível fazer é dizer para uma máquina específica: "Eu sei que este certificado não é de uma fonte oficialmente confiável, mas **eu, como administrador desta máquina, ordeno que você confie nele**."

Ao fazer isso, você está essencialmente tornando seu certificado de teste uma "autoridade raiz confiável" **apenas nesse computador**. O driver funcionará sem a necessidade do "Modo de Teste" e sem a marca d'água, mas apenas na(s) máquina(s) em que você realizar este procedimento manual.

Este método **não funcionará** para distribuir o driver para outros usuários.

### Passo a Passo para Instalar seu Certificado como Raiz Confiável

Você precisa instalar o seu certificado de teste (`WDKTestCert...`) no repositório de "Autoridades de Certificação Raiz Confiáveis" da **Máquina Local**.

**Passo 1: Exporte o Certificado (se você ainda não tiver o arquivo .cer)**

1.  Encontre seu arquivo `.sys` assinado.
2.  Clique com o botão direito > **Propriedades** > aba **Assinaturas Digitais**.
3.  Selecione a assinatura e clique em **Detalhes**.
4.  Na nova janela, clique em **Exibir Certificado**.
5.  Vá para a aba **Detalhes** e clique em **Copiar para Arquivo...**.
6.  Siga o assistente para exportar o certificado. Escolha o formato **"X.509 codificado em Base-64 (.CER)"**. Salve-o com um nome fácil, como `meu_certificado.cer`.

**Passo 2: Importe o Certificado para o Repositório Correto**

1.  Pressione **Windows + R**, digite `mmc` e pressione Enter para abrir o Console de Gerenciamento da Microsoft.
2.  No menu, clique em **Arquivo** > **Adicionar/Remover Snap-in...**.
3.  Na lista de snap-ins disponíveis, selecione **Certificados** e clique em **Adicionar >**.
4.  Uma janela perguntará para qual conta você quer gerenciar os certificados. **Esta é a parte mais importante:** selecione **Conta de computador** e clique em **Avançar**.
5.  Selecione **Computador local** e clique em **Concluir**.
6.  Clique em **OK** para fechar a janela de snap-ins.
7.  No console, expanda **Certificados (Computador Local)**.
8.  Expanda a pasta **Autoridades de Certificação Raiz Confiáveis**.
9.  Clique com o botão direito na subpasta **Certificados**, vá para **Todas as Tarefas** > **Importar...**.
10. O assistente de importação será aberto. Clique em **Avançar**.
11. Procure e selecione o arquivo `.cer` que você exportou no Passo 1.
12. Na tela "Repositório de Certificados", certifique-se de que a opção **"Colocar todos os certificados no repositório a seguir"** está selecionada e que o repositório é **"Autoridades de Certificação Raiz Confiáveis"**.
13. Clique em **Avançar** e depois em **Concluir**. Você receberá um aviso de segurança sobre a instalação de um certificado raiz. Como foi você quem o criou, pode clicar em **Sim**.

### Conclusão e Teste Final

1.  **Desative o Modo de Teste:** Agora que o Windows confia no seu certificado, você pode desligar o modo de teste. Abra o CMD como administrador e execute:
    ```cmd
    bcdedit /set testsigning off
    ```
2.  **Reinicie o computador.** A marca d'água deve desaparecer.
3.  Tente carregar seu driver novamente com `sc start StartSuspended`. Ele deve carregar sem erros.

**Aviso Importante sobre Secure Boot:**
Mesmo com este método, se a **Inicialização Segura (Secure Boot)** estiver ativada na BIOS/UEFI do computador, o Windows pode ainda assim bloquear o carregamento de drivers que não tenham uma assinatura da Microsoft (WHQL). Para desenvolvimento, muitas vezes é necessário desativar o Secure Boot ou usar drivers com assinatura de atestação da Microsoft.