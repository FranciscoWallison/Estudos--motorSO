### **Ferramentas Cyber para Burlar Apps Android**

Para modificar, burlar, ou analisar apps Android baixados da Play Store, as principais ferramentas e técnicas são:

#### **Análise Estática**

* **APKTool** – Descompila APKs para Smali (bytecode legível), ótimo para modificar lógica do app e recompilar.

[Apktool](https://github.com/iBotPeaches/Apktool)

[Video de exemplo](https://www.youtube.com/watch?v=ozWZYFFl_fw) 

---

* **JADX** – Decompila DEX para Java legível, facilitando leitura e compreensão do código.
* **Ghidra** – Para bibliotecas nativas .so (quando o app usa código em C/C++ via NDK).
* **IDA Pro/Radare2/Binary Ninja** – Alternativas para engenharia reversa de libs nativas ARM.

#### **Análise Dinâmica & Modificação em Runtime**

* **Frida** – Hooka funções Java ou nativas em tempo real. Permite modificar comportamentos do app enquanto ele roda (ex: bypass de root, login, pagamentos).
* **Xposed Framework** – Permite instalar módulos para alterar o funcionamento de apps/sistema Android sem modificar o APK. Precisa de root.
* **Magisk** – Framework moderno para root "sistêmico" e módulos de modificação, muito útil para esconder root e burlar root checks.
* **Objection** – Ferramenta all-in-one que usa Frida para pentest em apps Android/iOS (dumping, bypass root detection, etc).

#### **Debuggers**

* **Android Studio Debugger/ADB** – Para debug Java/Kotlin.
* **GDB, LLDB** – Para debug nativo ARM.
* **Frida** – Também pode ser usado como debugger dinâmico/hooker.

#### **Emuladores/VMs**

* **Genymotion/Android Studio Emulator** – Fáceis de rootar, ótimos para testar mods sem estragar aparelho real.
* **Nox/BlueStacks** – Emuladores Android populares, fáceis de mexer.

### 3. **Técnicas comuns para burlar apps**

* **Patchar APK** (remover/verificar lógicas, anúncios, root checks, etc)
* **Hookar funções** (modificar o comportamento durante a execução)
* **Fakear APIs (Mock)** – Manipular respostas de backend/API.
* **Utilizar módulos Magisk/Xposed** para enganar root checks, SafetyNet, etc.

---

#### **Resumo prático**

* Para APK: use **JADX, APKTool, Ghidra (para libs .so), Frida, Magisk, Xposed, Objection**.
* **x64dbg** não serve para APK, pois não lida com ARM/DEX, só PE/x86/x64.
* **Magisk** é o root moderno, mais furtivo, e aceita módulos de manipulação.
-----



### 1 ▪ Jadx

**O que é:** decompilador open-source que converte arquivos **DEX/APK** (byte-code Dalvik/ART) em **Java/Kotlin legível**.
**Para que serve:**

* Análise de segurança e auditoria de apps Android.
* “Inspect” rápido para entender bibliotecas closed-source.
* Conferir se informações sensíveis ficaram hard-coded no APK.

| Recurso                      | Detalhes práticos                                                                                                   |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| **Interface GUI e CLI**      | `jadx-gui` mostra árvore de pacotes, busca por string e hex-view; `jadx` (terminal) gera projeto completo no disco. |
| **Decompilação de Recursos** | Extrai XML, *drawables* e assets; recria `AndroidManifest.xml` com formatação original.                             |
| **Recomprilação opcional**   | Não recompila APK, mas gera código limpo que pode ser aberto no Android Studio para recompilar manualmente.         |
| **Suporte a Java 17/21**     | Versões ≥ 1.5 tratam opcodes mais novos e *record classes*.                                                         |
| **Plugins**                  | Script de destaque de patches, exportação para Ghidra, search avançado etc.                                         |

*Vantagens*

* **Fácil**: arraste-e-solte o APK e comece a ler.
* **Rápido**: decompila projetos grandes em segundos graças à engine multithread.
* **Licença Apache 2.0** – uso comercial liberado.

*Limitações*

* Código ofuscado por R8/ProGuard sai com nomes ‘a/b/c’.
* Blocos pesados em Kotlin Coroutines ou Dagger às vezes viram `goto/label`.
* Não cobre **native libs** (`.so`) – precisa de Ghidra/IDA para C/C++.

---

### 2 ▪ Bytecode Viewer (BCV)

(*Provavelmente o “byteviewer” que você citou; mantém o nome “Bytecode Viewer”*)

Ferramenta “tudo-em-um” para **classes Java/Kotlin, DEX e inclusive JARs/war**.

| Recurso                                   | Explicação                                                                                                               |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| **Múltiplos decompiladores side-by-side** | FernFlower, CFR, Procyon, Krakatau, Jadx-smali: compare resultados em abas.                                              |
| **Visualizador de byte-code**             | Abas “ASMifier” e “CFR-Bytecode” mostram instruções JVM (ILOAD, INVOKEVIRTUAL…) → ótimo para quem quer entender a stack. |
| **Inline-Hex & strings**                  | Destaca chunks literais para caça a credenciais.                                                                         |
| **Live patch**                            | Edite o `.java`, compile no próprio app e injete de volta no JAR/DEX.                                                    |
| **Suporte a plugins**                     | Ex.: buscador de *obfuscation patterns*, extrator de assets, exportador para JSON.                                       |
| **Função anti-tamper**                    | Gera patch para remover verificações de assinatura/cheats – útil em pesquisa, mas cuidado com aspectos legais.           |

*Vantagens*

* **Tudo em uma janela**: decompilar, ver byte-code, hex, editar, recompilar.
* **Comparar decompiladores** evita erros que um engine só mostraria.
* Roda via **Java FX** → multiplataforma.

*Desvantagens*

* Heavier: carrega todos os decompiladores, consome mais RAM.
* Atualizações menos frequentes (projeto comunitário desde 2014).
* Para APKs grandes, Jadx costuma ser mais rápido e estável.




#### **PCAP** Manipulação de rede [PCAPdroid](https://github.com/emanuele-f/PCAPdroid)