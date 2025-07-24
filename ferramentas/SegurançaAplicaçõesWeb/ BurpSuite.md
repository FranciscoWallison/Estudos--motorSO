O **Burp Suite** é uma ferramenta profissional de **teste de segurança de aplicações web**, amplamente utilizada por **pentesters**, **analistas de segurança** e **desenvolvedores** para identificar e explorar vulnerabilidades em sites e aplicações web.

### Principais funcionalidades do Burp Suite:

1. **Proxy Interceptador**:
   Permite capturar, modificar e redirecionar requisições HTTP/HTTPS entre o navegador e o servidor, ideal para analisar o tráfego de dados em tempo real.

2. **Spidering (Crawler)**:
   Mapeia automaticamente os endpoints de uma aplicação web, encontrando páginas e parâmetros ocultos.

3. **Scanner (versão Professional)**:
   Analisa a aplicação em busca de vulnerabilidades conhecidas como XSS, SQL Injection, CSRF, etc.

4. **Intruder**:
   Realiza ataques automatizados para testar a resistência da aplicação, como fuzzing ou força bruta.

5. **Repeater**:
   Permite repetir requisições HTTP editadas para testar manualmente diferentes parâmetros e respostas.

6. **Sequencer**:
   Analisa a aleatoriedade de tokens (como cookies ou tokens de sessão), ajudando a identificar previsibilidade.

7. **Comparer e Decoder**:
   Comparação de dados e codificação/decodificação em vários formatos (Base64, URL encoding, etc).

---

### Versões disponíveis:

* **Burp Suite Community (Gratuita)**: possui recursos limitados, ideal para aprendizado e testes manuais.
* **Burp Suite Professional (Paga)**: oferece funcionalidades avançadas como o scanner automático e ferramentas de automação.
* **Burp Suite Enterprise (Paga)**: voltada para integração em pipelines CI/CD, usada em ambientes corporativos com foco em testes contínuos.

---

### Exemplos de uso:

* Testar se um formulário de login é vulnerável a SQL Injection.
* Descobrir parâmetros ocultos que podem ser manipulados.
* Verificar se cookies estão sendo transmitidos com segurança (e.g., `HttpOnly`, `Secure`, etc).
* Automatizar fuzzing em campos de entrada.

Viceos 
[Dominando o Burp Suite](https://www.youtube.com/watch?v=qOJmTB_9-3g)