
## Como Fazer o Windows Confiar em um Certificado de Teste para Drivers (Nesta Máquina)

Ao desenvolver drivers para Windows, é comum usar **certificados de teste autoassinados**. No entanto, o Windows, por padrão, não confia nesses certificados. Ele os considera como não emitidos por uma **Autoridade de Certificação (CA) globalmente confiável**, como Microsoft, DigiCert ou GlobalSign.

**Não é possível "enganar" o Windows** para que ele trate um certificado de teste autoassinado como um certificado oficial. E isso é fundamental para a segurança: se fosse possível, qualquer pessoa poderia assinar malware com um certificado falso e o Windows o executaria sem questionar, comprometendo toda a segurança do sistema operacional. A validação criptográfica da cadeia de certificados é a base da confiança digital.

### A Solução: Tornar Seu Certificado Confiável *Nesta Máquina Específica*

O que **é possível** fazer é instruir uma máquina específica a confiar no seu certificado de teste. Você, como administrador, está essencialmente dizendo: "Eu sei que este certificado não vem de uma fonte oficialmente confiável, mas **eu ordeno que você confie nele nesta máquina**."

Ao fazer isso, seu certificado de teste se torna uma "autoridade raiz confiável" **apenas no computador onde você realiza este procedimento**. Isso permite que seu driver seja carregado sem a necessidade do "Modo de Teste" e sem a marca d'água no canto da tela.

**Importante:** Este método **não funciona** para distribuir o driver para outros usuários ou máquinas. Ele é exclusivo para o ambiente de desenvolvimento e testes.

-----

### Pré-requisitos (Comandos de Verificação Iniciais)

Antes de iniciar, se precisar verificar alguns aspectos do sistema, use estes atalhos:

  * **Logs do Windows (Sistema):** Pressione `Win + R`, digite `eventvwr.msc` e `Enter`.
  * **Gerenciador de Serviços do Windows:** Pressione `Win + R`, digite `services.msc` e `Enter`.
  * **Editor de Registro (Regedit):** Pressione `Win + R`, digite `regedit` e `Enter`.
      * Para drivers, a chave comum pode ser: `HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\StartSuspendedService` (se aplicável ao seu cenário).
  * **Gerenciador de Certificados Pessoais:** Pressione `Win + R`, digite `certlm.msc` e `Enter`.

-----

### Passo a Passo para Instalar seu Certificado como Raiz Confiável

Você precisa instalar o seu certificado de teste (geralmente com o prefixo `WDKTestCert...`) no repositório de "Autoridades de Certificação Raiz Confiáveis" da **Máquina Local**.

#### **Passo 1: Exporte o Certificado (se você ainda não tiver o arquivo .cer)**

Se você já tem o arquivo `.cer` do seu certificado de teste, pode pular este passo.

1.  Encontre seu arquivo `.sys` assinado (o arquivo do seu driver).
2.  Clique com o botão direito do mouse no arquivo e selecione **Propriedades**.
3.  Vá para a aba **Assinaturas Digitais**.
4.  Selecione a assinatura na lista e clique em **Detalhes**.
5.  Na nova janela de detalhes, clique em **Exibir Certificado**.
6.  Na janela do certificado, vá para a aba **Detalhes** e clique em **Copiar para Arquivo...**.
7.  Siga o assistente de exportação:
      * Clique em **Avançar**.
      * Escolha o formato **"X.509 codificado em Base-64 (.CER)"**.
      * Clique em **Avançar**.
      * Salve o arquivo com um nome fácil de lembrar, como `meu_certificado_driver.cer`, em um local acessível.
      * Clique em **Avançar** e depois em **Concluir**.

#### **Passo 2: Importe o Certificado para o Repositório Correto**

1.  Pressione `Windows + R`, digite `mmc` e pressione `Enter` para abrir o **Console de Gerenciamento da Microsoft**.
2.  No menu `Arquivo`, clique em **Adicionar/Remover Snap-in...**.
3.  Na lista de "Snap-ins disponíveis", selecione **Certificados** e clique em **Adicionar \>**.
4.  Uma nova janela "Snap-in de Certificados" aparecerá. **Esta é a etapa crucial:**
      * Selecione **Conta de computador** e clique em **Avançar**.
      * Selecione **Computador local (o computador em que este console está sendo executado)** e clique em **Concluir**.
5.  Clique em **OK** para fechar a janela "Adicionar ou Remover Snap-ins".
6.  No painel esquerdo do Console de Gerenciamento, expanda **Certificados (Computador Local)**.
7.  Expanda a pasta **Autoridades de Certificação Raiz Confiáveis**.
8.  Clique com o botão direito do mouse na subpasta **Certificados**, vá em **Todas as Tarefas** e selecione **Importar...**.
9.  O Assistente para Importação de Certificados será aberto. Clique em **Avançar**.
10. Clique em **Procurar...** e localize o arquivo `.cer` que você exportou no Passo 1 (ex: `meu_certificado_driver.cer`). Clique em **Abrir** e depois em **Avançar**.
11. Na tela "Repositório de Certificados", **certifique-se** de que a opção **"Colocar todos os certificados no repositório a seguir"** está selecionada e que o repositório é **"Autoridades de Certificação Raiz Confiáveis"**.
12. Clique em **Avançar** e depois em **Concluir**.
13. Você receberá um **Aviso de Segurança** sobre a instalação de um certificado raiz. Como você mesmo criou este certificado e está ciente de seu propósito, clique em **Sim**.

-----

### Conclusão e Teste Final

Com o certificado agora confiável em sua máquina, você pode desativar o modo de teste e verificar se o driver funciona corretamente.

1.  **Desative o Modo de Teste:**
      * Abra o **Prompt de Comando (CMD) como administrador**.
      * Execute o seguinte comando:
        ```cmd
        bcdedit /set testsigning off
        ```
2.  **Reinicie o computador.**
      * Após a reinicialização, a marca d'água do "Modo de Teste" deve ter desaparecido.
3.  **Tente carregar seu driver novamente:**
      * Abra o **Prompt de Comando (CMD) como administrador**.
      * Execute o comando para iniciar seu serviço de driver (substitua `StartSuspendedService` pelo nome real do seu serviço, se for diferente):
        ```cmd
        sc start StartSuspendedService
        ```
      * O driver deve carregar sem erros.

-----

### Aviso Importante sobre Secure Boot

Mesmo com este método, se a funcionalidade de **Inicialização Segura (Secure Boot)** estiver ativada na BIOS/UEFI do seu computador, o Windows ainda pode bloquear o carregamento de drivers que não possuem uma assinatura oficial da Microsoft (WHQL). Para fins de desenvolvimento e teste, muitas vezes é necessário **desativar o Secure Boot** nas configurações da BIOS/UEFI do computador, ou usar drivers com uma assinatura de atestação da Microsoft, se aplicável ao seu cenário.

-----

# Bypass de assinatura 

**kdmapper** :
---
      O kdmapper é uma ferramenta poderosa para quem precisa de um loader de drivers stealth, mas exige profundo entendimento de PE, kernel internals e, em geral, uma vulnerabilidade de kernel para fornecer o write-primitive. Use para pesquisa e testes de segurança — e sempre com cuidado para não comprometer a estabilidade do sistema.