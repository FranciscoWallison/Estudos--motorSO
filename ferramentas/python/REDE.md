
## 🧠 [**Objetivo do Script**](https://github.com/FranciscoWallison/Estudos---motorSO/blob/main/ferramentas/python/rede/script.py)

O script tem como finalidade **capturar pacotes de rede TCP**, especificamente trocados com os IPs `35.199.111.15` e `35.247.221.22`, registrá-los em **JSON estruturado** e exibir em tempo real em uma **interface gráfica** desenvolvida em `tkinter`.

---

## 📦 **Bibliotecas Utilizadas**

### 🔹 `scapy.all`
- **Função principal:** `sniff`
- **Descrição:** Captura pacotes da rede diretamente pela camada de enlace.
- **Camadas utilizadas:**
  - `IP`: Acesso a campos do protocolo IP (origem, destino).
  - `TCP`: Acesso a portas, flags e estado da conexão TCP.
  - `Raw`: Captura do conteúdo bruto do pacote (payload).

---

### 🔹 `datetime`
- **Função:** `datetime.now().isoformat()`
- **Descrição:** Gera timestamps legíveis e padronizados no formato ISO 8601.

---

### 🔹 `json`
- **Função:** `json.dumps`
- **Descrição:** Serializa os pacotes em formato JSON, facilitando o armazenamento e análise posterior.

---

### 🔹 `os`
- **Função:** `os.makedirs`
- **Descrição:** Garante que a pasta `logs/` exista para salvar os arquivos de log.

---

### 🔹 `hashlib`
- **Função:** `hashlib.md5`
- **Descrição:** Gera um hash MD5 único do payload, útil para **identificar pacotes duplicados** ou verificar integridade.

---

### 🔹 `string`
- **Função:** `string.printable`
- **Descrição:** Usado para filtrar se os bytes capturados são imprimíveis, para gerar **pré-visualizações seguras**.

---

### 🔹 `multiprocessing`
- **Objetos:** `Process`, `Queue`
- **Descrição:** Permite rodar uma **interface gráfica em paralelo** ao monitoramento de pacotes. A `Queue` serve como canal de comunicação entre os processos.

---

### 🔹 `viewer` (custom)
- **Função chamada:** `start_ui(queue)`
- **Descrição:** Interface tkinter que recebe os pacotes e exibe os dados em tempo real.

---

## 🧩 **Estrutura do Script**

### 1. `is_printable(data)`
Função auxiliar que verifica se todos os bytes de um dado são imprimíveis (evita renderizar dados binários em tela).

---

### 2. `parse_packet_factory(queue)`
Retorna uma função de callback (`parse_packet`) que será usada pelo `scapy.sniff`.

Essa função:
- Extrai informações dos pacotes IP/TCP.
- Calcula:
  - Direção (`ENVIADO` ou `RECEBIDO`),
  - Prévia ASCII,
  - Hash MD5,
  - Payload em hexadecimal.
- Armazena as informações no arquivo `logs/traffic.json`.
- Envia os dados para a interface em tempo real via `queue`.

---

### 3. `if __name__ == "__main__":`
Bloco principal do script que:
- Inicia uma fila `Queue()` para troca de dados entre processos.
- Cria e inicia um processo `ui_process` que roda a interface tkinter.
- Inicia a captura de pacotes com o filtro:
  ```
  filter="host 35.199.111.15 or host 35.247.221.22"
  ```
  Isso limita os pacotes monitorados para dois servidores-alvo específicos.
- Em caso de interrupção (`CTRL+C`), o processo da interface é encerrado.

---

## 🖼️ **Visualização**

O script depende de um módulo externo chamado `viewer.py`, que é responsável por:
- Criar a janela.
- Mostrar pacotes capturados em tempo real.
- Ter um botão para fechar a aplicação.

---

## ✅ **Resumo Final**

Este projeto combina **captura de pacotes em baixo nível** com **visualização em tempo real**, utilizando:
- `Scapy` para interceptar o tráfego da rede,
- `multiprocessing` para isolar a interface gráfica,
- `Tkinter` para fornecer uma visualização contínua dos dados.

### 📈 Aplicações:
- Engenharia reversa de jogos online.
- Análise forense de rede.
- Debugging de aplicações cliente-servidor.
- Identificação de comportamentos anômalos em conexões TCP.
