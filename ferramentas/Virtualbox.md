    Virtualização poderosa de código aberto
Para uso pessoal e empresarial

O VirtualBox é um software de virtualização completa de uso geral para hardware x86_64 (com versão 7.1 adicional para macOS/Arm), voltado para uso em laptops, desktops, servidores e embarcados.

[virtualbox](https://www.virtualbox.org/)

    Às vezes, quando você baixa software da Internet,
seu antivírus ou o Windows Defender pode alertá-lo dizendo que se trata de um malware.

    Na maioria das vezes, esses alertas são falsos positivos.

    Por exemplo, o Cheat Engine pode ser identificado como um vírus do tipo Cavalo de Troia,
assim como outros programas que utilizamos.

    Até mesmo alguns cracks que usaremos para praticar podem ser detectados
por determinados antivírus.

    Se você estiver preocupado com a possibilidade de realmente ser um vírus,
pode usar uma máquina virtual.

#### O que é uma máquina virtual?
Uma máquina virtual é um sistema que roda dentro do seu sistema operacional.

Basicamente, é como se você instalasse um sistema operacional dentro do seu sistema atual.

Existem algumas opções gratuitas de máquinas virtuais, como o VirtualBox, que pode ser baixado e instalado facilmente.

Depois de instalá-lo, você pode rodar um Windows dentro dele.

Caso queira resetar o sistema após cada sessão de hacking ou cracking, a máquina virtual permite que você volte rapidamente a um estado anterior.


Segue um passo-a-passo rápido para desativar o Secure Boot em diferentes produtos VMware. Escolha o que se aplica ao seu ambiente:

---

### VMware Workstation / VMware Player (Windows, Linux)

1. **Desligue completamente** a máquina virtual (não basta suspender).
2. Na biblioteca do VMware, **clique com o botão direito** na VM → **Settings…**.
3. Na janela de configurações, abra a aba **Options** → **Advanced**.
4. Em **Firmware Type**, verifique se **UEFI** está marcado.

   * Logo abaixo aparece a opção **Enable Secure Boot**. **Desmarque**-a.
5. Clique **OK** e inicie a VM.

> **Dica:** Se a caixa ficar esmaecida, feche o VMware, edite o arquivo `.vmx` da VM num editor de texto e adicione (ou altere) a linha:
> `uefi.secureBoot.enabled = "FALSE"`
> Salve e abra a VM novamente.

---

### VMware ESXi / vSphere Client

1. **Desligue** a VM.
2. Clique na VM → **Actions** → **Edit Settings**.
3. Selecione **VM Options** → **Boot Options**.
4. Em **Firmware**, deixe **EFI** selecionado e **desmarque** **Secure Boot**.
5. Salve e ligue a VM.

---

### Observações importantes

* Se o sistema operacional foi instalado **com** Secure Boot habilitado, ele pode exibir erros de assinatura ou até não iniciar após você desativá-lo; em geral basta recriar ou reparar o bootloader.
* Depois de desativar o Secure Boot, você já pode ativar o **Test Signing** no Windows convidado no CMD como admin (`bcdedit /set testsigning on`) para carregar drivers não assinados, como o *StartSuspended.sys*.
* Reative o Secure Boot somente quando não precisar mais de drivers de teste, para manter a segurança do ambiente.

Pronto! Assim sua VM fica com Secure Boot desativado e pronta para carregar drivers sem assinatura.
