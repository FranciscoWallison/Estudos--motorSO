## 🎯[ **Objetivo do Script**](https://github.com/FranciscoWallison/Estudos---motorSO/blob/main/ferramentas/python/bot/Script.py)
Este código monitora automaticamente a barra de vida (HP) do personagem no jogo *RPG Online*. Quando a vida está abaixo de um determinado limiar (ex: 70%), o script executa uma sequência de ações usando scripts [AutoHotkey](https://www.autohotkey.com/) (`.ahk`) para curar o personagem, mover o mouse e realizar cliques na interface do jogo.

---

## 📦 **Bibliotecas Utilizadas**

### 1. **`mss`**
Utilizada para capturar rapidamente uma região da tela (screenshot), permitindo verificar visualmente a barra de vida.

### 2. **`numpy`**
Processa os pixels da imagem capturada para determinar a quantidade de verde (indicando vida cheia).

### 3. **`pyautogui`**
Usada para obter a posição atual do mouse e validar se ele está na posição esperada para clicar.

### 4. **`subprocess`**
Executa scripts externos, neste caso, os scripts `.ahk` do AutoHotkey.

### 5. **`win32gui` e `win32con`**
Manipulam janelas do Windows, permitindo ativar o foco no jogo automaticamente.

### 6. **`os`, `time`, `datetime`**
Gerenciam caminhos de arquivos, tempo de execução e logs com carimbo de tempo.

---

## 🔧 **Principais Funções do Código**

### ✅ `calcular_percentual_vida()`
Captura a área da tela onde fica a barra de vida, conta quantos pixels são verdes e calcula a porcentagem de vida restante.

### ✅ `executar_ahk(script)`
Roda um script AutoHotkey específico — usado para ações como curar, mover o mouse ou clicar.

### ✅ `encontrar_janela(titulo_parcial)`
Localiza a janela do jogo pelo título (mesmo parcial), retornando seu handle (`hwnd`) para manipulação posterior.

### ✅ `ativar_janela(hwnd)`
Ativa e foca a janela do jogo no primeiro plano para garantir que as ações de teclado e mouse sejam recebidas.

### ✅ `mouse_esta_na_posicao()`
Confirma se o mouse está no local desejado com uma margem de erro, antes de clicar.

---

## ⚙️ **Lógica Principal**

1. **Valida se o AutoHotkey está instalado**.
2. **Procura continuamente a janela do RPG.**
3. Se encontrada:
   - Ativa a janela.
   - Verifica a porcentagem de vida do personagem.
   - Se a vida estiver abaixo de 70%:
     - Executa a sequência de cura via scripts AutoHotkey (`curar.ahk`, `mover_mouse.ahk`, `clicar.ahk`).
4. Se a janela não for encontrada, aguarda e tenta novamente.

---

## ✍️ **Resumo**

> O script implementa uma automação para o jogo *RPG Online*, monitorando a barra de vida do personagem via captura de tela (com `mss`) e processando a imagem com `numpy`. Com base na vida restante, ele aciona uma cadeia de comandos usando scripts AutoHotkey para curar o personagem. Essa automação é integrada ao sistema operacional usando bibliotecas como `pyautogui` e `win32gui`, fornecendo um exemplo prático de como interações com interfaces gráficas e jogos podem ser realizadas de forma programática.
