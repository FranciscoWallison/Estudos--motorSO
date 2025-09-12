Engenharia Reversa do Driver XignCode3 – Parte 1 – Identificando o Ponto de Entrada do Driver
Publicado em 8 de janeiro de 2020

Esta será uma série de postagens relacionadas ao Driver XignCode3 (XC3). Comecei isso no início de 2019, enquanto fazia minha pesquisa para o "Unveiling the underground world of Anti-Cheats" (Revelando o submundo dos Anti-Cheats). Por causa disso, você verá que a versão do XC3 não é a mais recente disponível.

Fazer a engenharia reversa de um Driver deste tipo não é apenas divertido, mas também permite que você aprenda muito sobre o funcionamento interno do Windows (Windows Internals). Dito isso, serei o mais breve e preciso possível, e tentarei anotar alguns destaques do que considero serem trechos de código ou recursos interessantes deste driver.

```cpp
NTSTATUS __stdcall DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS v4;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicLinkName;
    PDEVICE_OBJECT DeviceObject = 0;
    __int64 v6; // ctx

    if (!RegistryPath->Length || sub_140004A58(&v6) < 0)
        return STATUS_UNSUCCESSFUL;

    if (sub_140003A50(&SymbolicLinkName, &v6) < 0)
    {
        sub_140004AC0(&v6);
        return STATUS_UNSUCCESSFUL;
    }

    if (sub_1400039B0(&DeviceName, &v6) >= 0)
    {
        DriverObject->DriverUnload      = sub_1400004938;
        DriverObject->MajorFunction[0]  = &sub_1400045D0;  // IRP_MJ_CREATE
        DriverObject->MajorFunction[2]  = &sub_1400048580; // IRP_MJ_CLOSE
        DriverObject->MajorFunction[4]  =  sub_140004604;  // IRP_MJ_WRITE

        v4 = IoCreateDevice(DriverObject, 0, &DeviceName, 0x22u, 0, 0, &DeviceObject);
        if (v4 < 0)
        {
            sub_140004AC0(&v6);
            sub_140004AC0(&DeviceName);
            goto LABEL_10;
        }
        // (continua fora do recorte)
    }

LABEL_10:
    return v4;
}
```

**O que abordaremos?**

  * Aprender sobre a `DriverEntry`
  * Aprender a identificar esta função no binário
  * Identificar como o `Driver Object` é criado
  * Analisar as `Major functions` (funções principais) implementadas, especialmente a de `dispatcher` (despachante)
  * Detectar algum código adicional, que não sabemos o que faz. Ainda.

Você também pode encontrar **aqui** o resultado final da engenharia reversa da função `DriverEntry`.

**Por onde eu começo?**
Obtendo o arquivo `.sys`, é claro. Se você quiser fazer a engenharia reversa por conta própria, estes são os hashes do arquivo que eu usei:

md5: C82DE0BC9A3A08E351B9BFA960F8757A

sha1: F3AE1406B4CD7D1394DAC529C62BB01F3EB46D4A

sha256: E7994B56152955835B5025049ABEE651E9C52A6CCAD7E04F2C458C8568967245


Quando quero fazer a engenharia reversa de um Driver, geralmente gosto de começar pela `DriverEntry`. Nesta função, podemos encontrar alguns detalhes interessantes que nos ajudarão a identificar a maioria dos recursos que um driver implementou.

`IDA Pro` e `Ghidra` geralmente conseguem detectar essa função automaticamente. Mas, às vezes, você pode ter que fazer isso manualmente. Como o **site** da Microsoft diz:

> “A `DriverEntry` é a primeira rotina fornecida pelo driver que é chamada após o carregamento de um driver. Ela é responsável por inicializar o driver.”

Basicamente, precisamos procurar por uma função que chame `IoCreateDevice`, isso geralmente é feito dentro da `DriverEntry`, e é usado para criar um novo objeto de dispositivo (`device object`). Esta é apenas uma maneira de fazer isso, mas como o IDA fez isso automaticamente para nós, vamos pular direto para a fase de engenharia reversa.

**DriverEntry (0x1400047B8)**
Para manter esta postagem limpa, você pode ver **aqui** como a `DriverEntry` foi descompilada pelo IDA Pro no início, e **aqui** o assembly. Eu o encorajo a tirar alguns minutos para ler o código e tentar identificar o objetivo principal da função antes de continuar com a leitura. Vamos ver como podemos limpar isso. Às vezes, renomear e alterar os tipos das variáveis pode ser 50% do trabalho e, claro, nos ajudará a entender melhor a função.

Sabemos que a `DriverEntry` recebe dois parâmetros: um ponteiro `_DRIVER_OBJECT` e uma `PUNICODE_STRING` com o caminho do registro. Portanto, vamos alterar o nome e o tipo dessas duas variáveis.

Algo a se levar em conta é que muitos dos tipos de estrutura de que precisamos ao fazer engenharia reversa não estão disponíveis por padrão no IDA Pro. Portanto, podemos importá-los fazendo: “File-\>Load File-\>Parse C Header file…”.

Voltando ao código, temos algumas validações e a concatenação do `SymbolicLinkName` e do `DeviceName`, nada estranho. Eles estão apenas configurando as strings necessárias:

```cpp
	_DriverObject = DriverObject;
	DeviceObject = 0i64;
	if (!RegistryPath->Length || fn_strcat(Dest, RegistryPath) < 0)
		return 0xC0000001i64;
	if (sub_140003A50(&SymbolicLinkName, Dest) < 0)
	{
		Real_Driver_Entry(Dest);
		return 0xC0000001i64;
	}
```

A `DriverEntry` precisa implementar algumas “responsabilidades ou rotinas obrigatórias” específicas, conforme explicado **aqui**. Vamos ver como eles fizeram isso antes de chamar `IoCreateDevice`:

```cpp
_DriverObject->DriverUnload = fn_DriverUnloadDispatcher;
_DriverObject->MajorFunction[0] = fn_DispatchCreate;       // IRP_MJ_CREATE
_DriverObject->MajorFunction[2] = fn_DispatchClose;        // IRP_MJ_CLOSE                        
_DriverObject->MajorFunction[4] = fn_DriverIOCTLDispatcher;  // IRP_MJ_WRITE                        
ntStatus = IoCreateDevice(_DriverObject, 0, &DeviceName, 0x22u, 0, 0, &DeviceObject);
```

Eu renomeei algumas variáveis e funções para descrever seu propósito principal. No entanto, deixe-me explicar como você pode identificá-las facilmente:

**Aqui** podemos ver todos os Códigos de Função Principal de IRP (`IRP Major Function Codes`) disponíveis no Windows. No trecho de código anterior, apenas os números 0, 2 e 4 foram implementados:

  * `IRP_MJ_CREATE` 0x00
  * `IRP_MJ_CLOSE` 0x02
  * `IRP_MJ_WRITE` 0x04

Uma lista completa pode ser encontrada no meu **github**.

`IRP_MJ_CREATE` e `IRP_MJ_CLOSE` são apenas funções genéricas. A interessante é a `IRP_MJ_WRITE`, que lidará com as requisições vindas do modo de usuário (`user-mode`) e está implementada em `fn_DriverIOCTLDispatcher`. Explicarei em outra postagem como essa função funciona e como ela despacha cada requisição IRP (`IRP request`). Por enquanto, vamos focar nas funções interessantes que a `DriverEntry` chama.


```cpp
__int64 __fastcall DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	PDRIVER_OBJECT _DriverObject; // rdi
	signed int ntStatus; // ebx
	UNICODE_STRING *v5; // rcx
	char Dest[16]; // [rsp+40h] [rbp-30h]
	UNICODE_STRING DeviceName; // [rsp+50h] [rbp-20h]
	UNICODE_STRING SymbolicLinkName; // [rsp+60h] [rbp-10h]
	PDEVICE_OBJECT DeviceObject; // [rsp+88h] [rbp+18h]

	_DriverObject = DriverObject;
	DeviceObject = 0i64;
	if (!RegistryPath->Length || fn_strcat(Dest, RegistryPath) < 0)
		return 0xC0000001i64;
	if (sub_140003A50(&SymbolicLinkName, Dest) < 0)
	{
		Real_Driver_Entry(Dest);
		return 0xC0000001i64;
	}
	if (sub_1400039B0(&DeviceName, Dest) >= 0)
	{
		_DriverObject->DriverUnload = fn_DriverUnloadDispatcher;
		_DriverObject->MajorFunction[0] = fn_DispatchCreate;// IRP_MJ_CREATE
		_DriverObject->MajorFunction[2] = fn_DispatchClose;// IRP_MJ_CLOSE                        
		_DriverObject->MajorFunction[4] = fn_DriverIOCTLDistpacher;// IRP_MJ_WRITE                        
		ntStatus = IoCreateDevice(_DriverObject, 0, &DeviceName, 0x22u, 0, 0, &DeviceObject);
		if (ntStatus < 0)
		{
			Real_Driver_Entry(Dest);
			Real_Driver_Entry(&DeviceName);
			goto LABEL_10;
		}
		DeviceObject->Flags |= 4u;
		_mm_storeu_si128(&xmmword_14000CD78, *Dest);
		_mm_storeu_si128(&::SymbolicLinkName, SymbolicLinkName);
		fn_InitDispatchMethodArray();
		ntStatus = fn_InitRegistrationNotifyAndCallbackRoutines();
		if (ntStatus >= 0)
		{
			ntStatus = fn_ObtainKernelFunctions();
			if (ntStatus >= 0)
			{
				ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
				if (ntStatus >= 0)
				{
					ntStatus = 0;
					goto LABEL_18;
				}
				nullsub_1();
			}
			fn_RegisterCreateProcessNotifyRoutine();
			nullsub_1();
		}
		IoDeleteDevice(DeviceObject);
	LABEL_18:
		v5 = &DeviceName;
		goto LABEL_19;
	}
	Real_Driver_Entry(Dest);
	ntStatus = 0xC0000001;
LABEL_10:
	v5 = &SymbolicLinkName;
LABEL_19:
	Real_Driver_Entry(v5);
	return ntStatus;
}
```


-----

Após alguma análise do comportamento interno das funções que estão sendo chamadas, chegamos ao seguinte código:

```cpp
		fn_InitDispatchMethodArray();
		ntStatus = fn_InitRegistrationNotifyAndCallbackRoutines();
		if (ntStatus >= 0)
		{
			ntStatus = fn_ObtainKernelFunctions();
			if (ntStatus >= 0)
			{
				ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
				if (ntStatus >= 0)
				{
					ntStatus = 0;
					goto LABEL_18;
				}
				nullsub_1();
			}
			fn_RegisterCreateProcessNotifyRoutine();
			nullsub_1();
		}
```

Alguns nomes interessantes apareceram:

  * `fn_InitDispatchMethodArray`
  * `fn_InitRegistrationNotifyAndCallbackRoutines`
  * `fn_ObtainKernelFunctions`
  * `fn_RegisterCreateProcessNotifyRoutine`


```cpp
InitDispatchMethodArray();

ntStatus = InitRegistrationNotifyAndCallbackRoutines();
if (ntStatus >= 0) {
    ntStatus = ObtainKernelFunctions();
    if (ntStatus >= 0) {
        ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
        if (ntStatus >= 0) {
            ntStatus = STATUS_SUCCESS;
            goto done;
        }
        nullsub_1(); // placeholder
    }
}

RegisterCreateProcessNotifyRoutine(); // executa mesmo se acima falhar
nullsub_1();

done:
return ntStatus;
```


***

Como você já deve ter percebido, eu costumo escrever nomes bem descritivos, haha. Esses nomes podem estar errados ou não, dependendo do que descobrirmos mais adiante durante o processo de engenharia reversa. Como eu mencionei, a maior parte deste trabalho foi feita meses atrás. Às vezes, a engenharia reversa envolve muita suposição, então precisamos começar a imaginar o que cada função poderia potencialmente fazer e dar nomes a elas para que possamos visualizar o objetivo de cada uma delas.

Os nomes são baseados no que pude inferir sobre essas funções. Vamos analisá-las a fundo nas próximas postagens.

Basicamente, podemos separá-las em dois grupos com base em seus objetivos. As funções que registram algum tipo de callback: `fn_InitRegistrationNotifyAndCallbackRoutines` e `fn_RegisterCreateProcessNotifyRoutine`. E as funções que inicializam as informações básicas que o Driver precisa para funcionar: `fn_InitDispatchMethodArray` e `fn_ObtainKernelFunctions`.

### Próximos Passos
* Analisando as funções de inicialização (`fn_InitDispatchMethodArray` e `fn_ObtainKernelFunctions`)
* Analisar a função de `Dispatch` (`fn_DriverIOCTLDispatcher`)
* Analisando as Rotinas de Registro de Notificação e Callback (`fn_InitRegistrationNotifyAndCallbackRoutines` e `fn_RegisterCreateProcessNotifyRoutine`)