O **kdmapper** (Kernel Driver Mapper) é uma técnica e uma ferramenta para carregar “na mão” um driver de modo-kernel no Windows, contornando o Driver Signature Enforcement (DSE) e sem usar o serviço padrão de `ZwLoadDriver`. Em vez disso, o próprio usuário mapeia o binário do driver diretamente na memória do kernel, resolvendo relocations e imports, e executa seu `DriverEntry`.

---

## 1. Para que serve

* **Bypass de assinatura**: permite carregar drivers não-assinados em máquinas com DSE ativo.
* **Stealth**: o driver não é inserido em `PsLoadedModuleList`, dificultando sua detecção por AVs ou EDRs.
* **Flexibilidade**: você pode carregar payloads arbitrários no kernel sem depender das APIs oficiais de “service”.

---

## 2. Pré-requisitos

1. **Privilégios de administrador**: necessário para abrir handles e chamar Nt/Zw.
2. **Primitiva de escrita no kernel** (opcional, mas comum): muitos mappers usam um driver “vulnerável” (ex: o famoso `capcom.sys`) para obter um *write-what-where* que permite alocar e escrever em memória kernel.
3. Binário do driver (“payload”) compilado para o mesmo alvo (x86/x64) e para a mesma versão de Windows.

---

## 3. Passo a passo geral

1. **Abrir o arquivo do driver**

   * `CreateFile` no `.sys` e `CreateSection` com `SEC_IMAGE`.
2. **Mapear seção em user-mode**

   * `NtMapViewOfSection` para ter acesso ao PE no espaço de usuário.
3. **Alocar memória no kernel**

   * Usando a primitiva de escrita (ou chamadas não-documentadas), aloca um bloco no pool de sistema (`ExAllocatePool`).
4. **Copiar headers e seções**

   * Memcpy dos headers + cada `.text/.data/.rsrc` para os offsets corretos na região kernel.
5. **Aplicar relocations**

   * Se a imagem não foi carregada no seu `PreferredImageBase`, corrige todos os delta entries na tabela de relocations.
6. **Resolver imports**

   * Para cada import, busca o endereço nas exports de `ntoskrnl.exe` (ou outros drivers dependidos) e preenche a IAT.
7. **Chamar DriverEntry**

   * Cria uma thread no kernel (via `PsCreateSystemThread`) apontando para o ponto de entrada do driver, passando `DriverObject` e `RegistryPath`.
8. **Limpar vestígios**

   * Opcionalmente remove o objeto de seção ou zera cabeçalhos para dificultar dumps.
---

## 4. Vantagens e riscos

* **Vantagens**

  * Carregamento totalmente customizável.
  * Menos “ruído” no kernel (pode ocultar o driver de listas).
* **Riscos**

  * Incompatibilidade entre versões de Windows (offsets/exports mudam).
  * Estabilidade: se relocations ou imports falharem, causa BSOD.
  * Legalidade e detecção: muitas soluções de segurança detectam padrões de manual mapping.

---

### Conclusão

O **kdmapper** é uma ferramenta poderosa para quem precisa de um loader de drivers stealth, mas exige profundo entendimento de PE, kernel internals e, em geral, uma vulnerabilidade de kernel para fornecer o write-primitive. Use para pesquisa e testes de segurança — e sempre com cuidado para não comprometer a estabilidade do sistema.
