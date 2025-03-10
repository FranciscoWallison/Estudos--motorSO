---

# **📌 Passo a Passo para Configurar o Ambiente para Kernel-Mode Driver (KMDF)**

## **✅ 1. Instalar o Visual Studio 2022**
1. **Baixar o Visual Studio 2022** (Community, Professional ou Enterprise)  
   - 🔗 [Baixar Visual Studio 2022](https://visualstudio.microsoft.com/pt-br/downloads/)
2. **Abrir o instalador** e escolher as seguintes opções:
   - **C++ para desenvolvimento de desktop**
   - **Desenvolvimento para drivers do Windows**
3. **Finalizar a instalação e reiniciar o computador.**

---

## **✅ 2. Instalar os Pacotes Necessários**
Agora, instale os pacotes obrigatórios para compilar **drivers KMDF**.

### **📌 No Visual Studio Installer:**
1. **Abra o Visual Studio Installer** (`Win + S`, procure por *Visual Studio Installer*).
2. **Clique em "Modificar" na versão do Visual Studio instalada**.
3. **Vá até a aba "Componentes Individuais"** e marque os seguintes pacotes:

#### **🔹 Compiladores e Bibliotecas:**
- ✅ **MSVC v143 - VS 2022 C++ ARM64/ARM64EC Spectre-mitigated libs**
- ✅ **MSVC v143 - VS 2022 C++ x64/x86 Spectre-mitigated libs**
- ✅ **ATL do C++ para ferramentas de build v143 com Mitigações de Espectro (ARM64/ARM64EC)**
- ✅ **ATL do C++ para ferramentas de build v143 com Mitigações de Espectro (x86 & x64)**
- ✅ **C++ MFC para ferramentas de build v143 com Mitigações de Espectro (ARM64/ARM64EC)**
- ✅ **C++ MFC para ferramentas de build v143 com Mitigações de Espectro (x86 & x64)**

#### **🔹 Ferramentas de Desenvolvimento de Drivers:**
- ✅ **Windows Driver Kit (WDK)**
- ✅ **Windows 10 SDK** *(versão mais recente)*
- ✅ **Windows 11 SDK** *(se disponível)*

4. **Clique em "Modificar" e aguarde a instalação.**
5. **Reinicie o computador para aplicar as mudanças.**

---

## **✅ 3. Criar um Novo Projeto KMDF**
1. **Abra o Visual Studio 2022** como **Administrador**.
2. Clique em **"Criar um Novo Projeto"**.
3. Pesquise por **"Kernel-Mode Driver (KMDF)"**.
4. **Selecione "Kernel-Mode Driver (KMDF)"** e clique em **Avançar**.
5. **Escolha um nome para o projeto** (exemplo: `MeuDriverKMDF`).
6. **Clique em Criar.**

Agora o Visual Studio criará um projeto KMDF vazio.

---

## **✅ 4. Instalar Dependências via NuGet**
1. **No Gerenciador de Soluções**, clique com o **botão direito no nome do projeto**.
2. **Selecione "Gerenciar Pacotes NuGet"**.
3. **Na aba "Procurar"**, pesquise por **WDK**:
   - 🔹 `Microsoft.Windows.SDK.CPP.10.0.26100.2454`
   - 🔹 `Microsoft.Windows.SDK.CPP.x64.10.0.26100.2454`
   - 🔹 `Microsoft.Windows.WDK.x64.10.0.26100.2454`
4. **Clique em "Instalar" para cada um dos pacotes**.
5. **Aguarde a instalação** e feche o Gerenciador de Pacotes NuGet.

---

## **✅ 5. Compilar o Projeto**
Agora, vamos **compilar o driver KMDF**.

1. **No Visual Studio**, pressione:
   ```
   Ctrl + Shift + B
   ```
   ou vá até **Compilar → Compilar Solução**.

2. **Se houver erros sobre `Driver.tmh`, siga o próximo passo.**

---

## **✅ 6. Resolver Erros Relacionados ao `driver.tmh`**
Se houver erros do tipo:
```
Erro (ativo) E1696 não é possível abrir o arquivo fonte "driver.tmh"
```
Siga estes passos:

1. **Navegue até a pasta de saída da compilação**:
   ```
   x64/Debug/
   ```
2. **Procure pelo arquivo `driver.tmh`**.
3. **Adicione `driver.tmh` ao projeto**:
   - **Clique com o botão direito no projeto** → **Adicionar → Arquivo Existente**.
   - Selecione `driver.tmh` e clique em **Adicionar**.
4. **Agora, tente compilar novamente (`Ctrl + Shift + B`)**.

---

# **🚀 Conclusão**
Agora, seu ambiente está completamente configurado para **desenvolver e compilar Kernel-Mode Drivers (KMDF)**.

✅ **Visual Studio 2022 instalado**  
✅ **Todos os pacotes essenciais baixados e instalados**  
✅ **Projeto KMDF criado corretamente**  
✅ **Dependências do WDK instaladas via NuGet**  
✅ **Compilação funcionando corretamente**  





---

Agora que você tem seu **Kernel-Mode Driver (KMDF)** configurado corretamente, podemos implementar a lógica para **monitorar processos no kernel** e **identificar quando "Tutorial-i386.exe" for executado**.

---

# **📌 Como Monitorar "Tutorial-i386.exe" no Kernel (Ring 0)**
Para capturar processos no **Ring 0**, usaremos a API do Windows:
- ✅ **`PsSetCreateProcessNotifyRoutineEx()`** → Registra um callback para monitorar processos quando são criados ou finalizados.
- ✅ **`DbgPrintEx()`** → Exibe informações no **DebugView**.

---

## **📌 1. Código Completo do Driver**
### **📄 Arquivo `Driver.c`**
Substitua o conteúdo do `Driver.c` pelo seguinte código:

```c
#include <ntddk.h>
#include <wdf.h>

// Declaração da função de callback para monitoramento de processos
VOID ProcessNotifyEx(
    PEPROCESS Process,
    HANDLE ProcessId,
    PPS_CREATE_NOTIFY_INFO CreateInfo
);

// Função chamada quando o driver é descarregado
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    
    // Remove o callback de monitoramento de processos
    PsSetCreateProcessNotifyRoutineEx(ProcessNotifyEx, TRUE);
    
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[KMDF-DRIVER] Driver Descarregado!\n");
}

// Função principal do driver (chamada ao carregar)
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[KMDF-DRIVER] Driver Iniciado!\n");

    // Registra a função de callback para monitorar processos
    NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyEx, FALSE);
    if (!NT_SUCCESS(status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[KMDF-DRIVER] Falha ao registrar o callback de processos!\n");
        return status;
    }

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}

// Função chamada sempre que um processo é criado ou finalizado
VOID ProcessNotifyEx(
    PEPROCESS Process,
    HANDLE ProcessId,
    PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);

    if (CreateInfo != NULL) // Processo foi criado
    {
        if (CreateInfo->ImageFileName)
        {
            // Verifica se o nome do processo contém "Tutorial-i386.exe"
            if (wcsstr(CreateInfo->ImageFileName->Buffer, L"Tutorial-i386.exe"))
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                           "[KMDF-DRIVER] Processo Detectado: %ws (PID: %d)\n",
                           CreateInfo->ImageFileName->Buffer, (ULONG)(ULONG_PTR)ProcessId);
            }
        }
    }
}
```

---

## **📌 2. Compilar o Driver**
Agora que temos o código pronto, siga os passos para **compilar e instalar** o driver.

1. **No Visual Studio**, pressione:
   ```
   Ctrl + Shift + B
   ```
   ou vá até **Compilar → Compilar Solução**.

2. Se a compilação for bem-sucedida, o arquivo do driver será gerado em:
   ```
   x64/Debug/MeuDriverKMDF.sys
   ```
---

# Meios para bular assinatura de driver:

[Desabilitar Imposição De Assinatura De Drivers No Windows 11](https://mspoweruser.com/driver-signature-enforcement-windows-11/?language=pt)

[Criar um certificado público autoassinado para autenticar seu aplicativo](https://learn.microsoft.com/pt-br/entra/identity-platform/howto-create-self-signed-certificate)

### **✅ Instalar e Iniciar o Driver**
Agora, vamos **instalar e rodar** o driver manualmente.

1. **Copie o arquivo `.sys` para `C:\Windows\System32\drivers\`**
2. **No Prompt de Comando (Admin), registre o driver:**
   ```sh
   sc create MonitorProcessoKMDF type= kernel binPath= "C:\Windows\System32\drivers\MeuDriverKMDF.sys"
   ```
3. **Inicie o driver:**
   ```sh
   sc start MonitorProcessoKMDF
   ```

---

## **📌 4. Ver os Logs no DebugView**
Agora que o driver está rodando, vamos verificar se ele está capturando o processo.

1. **Baixe o DebugView**:  
   🔗 [Download DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview)
2. **Execute como Administrador**.
3. **Ative a captura de Kernel** (`Capture → Capture Kernel`).
4. **Abra o Gerenciador de Tarefas** (`Ctrl + Shift + Esc`) e **execute o `Tutorial-i386.exe`**.
5. No DebugView, você verá uma saída como esta:
   ```
   [KMDF-DRIVER] Processo Detectado: Tutorial-i386.exe (PID: 1234)
   ```

---

## **📌 5. Remover o Driver**
Se precisar remover o driver do sistema:

```sh
sc stop MonitorProcessoKMDF
sc delete MonitorProcessoKMDF
```

Se quiser **desativar o Test Mode**:

```sh
bcdedit /set testsigning off
shutdown /r /t 0
```

---

## **🚀 Conclusão**
Agora, seu **Kernel-Mode Driver (KMDF)**:
✅ **Monitora processos no Ring 0**  
✅ **Captura a criação do processo "Tutorial-i386.exe"**  
✅ **Imprime a identificação do processo no DebugView**  

Se precisar **bloquear** o processo, me avise que podemos adicionar essa funcionalidade! 🚀💻