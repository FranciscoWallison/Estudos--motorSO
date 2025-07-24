### 🔐 **O que é a OWASP?**

A **OWASP (Open Worldwide Application Security Project)** é uma **fundação sem fins lucrativos** que tem como objetivo **melhorar a segurança de software**. Ela é reconhecida mundialmente por promover conhecimento aberto e gratuito sobre segurança em aplicações.

---

### 🛠️ **Como a OWASP atua**

A OWASP atua por meio de:

* **Projetos abertos e colaborativos** (como guias, ferramentas e listas).
* **Conferências e eventos** voltados à comunidade de segurança.
* **Capítulos locais** espalhados pelo mundo que promovem encontros, palestras e workshops.
* **Educação e treinamento** em segurança de software.

Tudo é **gratuito e acessível**, o que facilita o aprendizado e colaboração da comunidade.

---

### 📁 **Projetos da OWASP**

A OWASP possui diversos projetos, divididos em categorias como documentação, ferramentas e padrões. Os mais famosos incluem:

* **OWASP Top 10**: lista com os 10 principais riscos de segurança em aplicações web.
* **OWASP ASVS (Application Security Verification Standard)**: um padrão para testar segurança de aplicações.
* **OWASP ZAP (Zed Attack Proxy)**: uma ferramenta gratuita para testar vulnerabilidades em aplicações web.
* **OWASP Mobile Top 10**: os principais riscos de segurança para aplicativos móveis.
* **Cheat Sheets**: guias práticos e objetivos sobre diversos temas de segurança (ex: autenticação, criptografia).

---

### 🤝 **Como participar da OWASP**

Qualquer pessoa pode participar! Formas de contribuir:

* **Traduzir e revisar documentos.**
* **Criar ou colaborar com projetos.**
* **Participar de eventos e capítulos locais.**
* **Contribuir com código ou relatórios de vulnerabilidades.**
* **Fazer doações** (opcional).

Você não precisa ser especialista. A OWASP valoriza **colaboradores iniciantes e experientes**.

---

### 🌍 **Capítulos da OWASP**

Os **capítulos** são comunidades locais que organizam **eventos, meetups e treinamentos**. Exemplo:

* OWASP São Paulo
* OWASP Recife
* OWASP Lisboa

Você pode procurar o capítulo mais próximo no site oficial da OWASP e participar dos encontros (muitos são gratuitos e online).

---

### 🧨 **OWASP Top 10**

É o projeto mais conhecido da OWASP. Ele lista as **10 falhas de segurança mais críticas em aplicações web**, com base em dados do mundo real.

A versão mais recente (2021) inclui:

1. **Broken Access Control** (controle de acesso quebrado)
2. **Cryptographic Failures** (falhas criptográficas)
3. **Injection** (ex: SQL Injection)
4. **Insecure Design** (design inseguro)
5. **Security Misconfiguration** (má configuração de segurança)
6. **Vulnerable and Outdated Components**
7. **Identification and Authentication Failures**
8. **Software and Data Integrity Failures**
9. **Security Logging and Monitoring Failures**
10. **Server-Side Request Forgery (SSRF)**

Cada item traz **exemplos práticos, impactos e recomendações** para evitar essas falhas.

---


Claro! Abaixo está uma explicação detalhada de cada uma das 10 categorias do **OWASP Top 10 (2021)** com **exemplos práticos** para facilitar a compreensão:

---

### 1. 🔓 **Broken Access Control (Controle de Acesso Quebrado)**

**O que é:** Quando usuários podem acessar dados ou funções que deveriam estar restritos.

**Exemplo prático:**
Um site de administração com URL `example.com/admin/painel` não verifica se o usuário é administrador. Basta qualquer usuário logado digitar essa URL e ele acessa o painel.

**Como evitar:**

* Verificar permissões em todas as requisições.
* Esconder e proteger rotas sensíveis no backend.
* Usar controles baseados em função (RBAC).

---

### 2. 🔐 **Cryptographic Failures (Falhas Criptográficas)**

**O que é:** Uso incorreto ou falta de criptografia para proteger dados sensíveis.

**Exemplo prático:**
Um formulário de login envia nome de usuário e senha em texto claro (sem HTTPS), permitindo que atacantes os capturem com um sniffer.

**Como evitar:**

* Sempre usar HTTPS.
* Criptografar senhas com algoritmos como bcrypt/scrypt/Argon2.
* Nunca reinventar algoritmos criptográficos.

---

### 3. 💉 **Injection (Injeção)**

**O que é:** Entrada de dados maliciosos que alteram comandos do sistema ou banco de dados.

**Exemplo prático:**

```sql
SELECT * FROM usuarios WHERE nome = '$nome';
```

Se o usuário digitar `admin' --`, a consulta vira:

```sql
SELECT * FROM usuarios WHERE nome = 'admin' --';
```

E ele pode acessar como "admin" sem senha.

Payload's de acessos para invadir pode ser feticom só enviando ```'```

**Como evitar:**

* Usar prepared statements (consultas parametrizadas).
* Validar e sanitizar entradas.

---

### 4. 🧱 **Insecure Design (Design Inseguro)**

**O que é:** Arquitetura ou lógica de aplicação que facilita falhas de segurança, mesmo sem bugs diretos.

**Exemplo prático:**
Um sistema de recuperação de senha envia a senha original por e-mail em texto claro. Isso indica que a senha é armazenada sem criptografia.

**Como evitar:**

* Aplicar princípios de design seguro desde o início do projeto.
* Fazer threat modeling.
* Evitar decisões arquitetônicas perigosas (ex: lógica de negócios no frontend).

---

### 5. ⚙️ **Security Misconfiguration (Má Configuração de Segurança)**

**O que é:** Configurações padrão, serviços desnecessários expostos ou mensagens de erro que revelam detalhes do sistema.

**Exemplo prático:**
Um servidor Apache exibe páginas de erro com a versão do software e sistema operacional — informações úteis para atacantes.

**Como evitar:**

* Desabilitar diretórios listáveis e mensagens de erro detalhadas.
* Configurar permissões adequadas em arquivos e servidores.
* Fazer hardening da infraestrutura.

---

### 6. 📦 **Vulnerable and Outdated Components (Componentes Vulneráveis ou Desatualizados)**

**O que é:** Uso de bibliotecas ou sistemas com vulnerabilidades conhecidas.

**Exemplo prático:**
Uma aplicação usa a biblioteca jQuery 1.6, vulnerável a XSS, mesmo havendo versões mais seguras disponíveis.

**Como evitar:**

* Manter dependências atualizadas.
* Usar ferramentas como OWASP Dependency-Check ou Snyk.
* Remover componentes não utilizados.

---

### 7. 🆔 **Identification and Authentication Failures (Falhas de Identificação e Autenticação)**

**O que é:** Autenticação fraca, sessão insegura, senhas previsíveis ou ausência de bloqueio após tentativas.

**Exemplo prático:**
A aplicação permite infinitas tentativas de login sem bloqueio ou CAPTCHA, facilitando brute-force.

**Como evitar:**

* Implementar autenticação forte (MFA).
* Bloquear ou atrasar após tentativas repetidas.
* Usar cookies de sessão seguros (`HttpOnly`, `Secure`, `SameSite`).

---

### 8. 🧩 **Software and Data Integrity Failures (Falhas de Integridade de Software e Dados)**

**O que é:** Atualizações, bibliotecas ou dados que podem ser manipulados sem verificação de integridade.

**Exemplo prático:**
Uma aplicação baixa um plugin JavaScript de uma URL externa sem verificação de integridade, e essa URL foi comprometida.

**Como evitar:**

* Usar assinaturas digitais e hashes (SRI).
* Garantir a integridade dos pacotes e dependências.
* Proteger pipelines de CI/CD.

---

### 9. 📉 **Security Logging and Monitoring Failures (Falhas de Log e Monitoramento de Segurança)**

**O que é:** Ausência de logs ou monitoramento que permita detectar e responder a incidentes de segurança.

**Exemplo prático:**
Um invasor faz várias tentativas de SQL Injection sem ser detectado, pois o sistema não registra falhas de entrada ou alertas.

**Como evitar:**

* Gerar logs com informações de segurança.
* Monitorar e alertar sobre comportamentos suspeitos.
* Armazenar logs com acesso restrito e persistente.

---

### 10. 🔄 **Server-Side Request Forgery (SSRF)**

**O que é:** Um servidor é induzido a fazer requisições para outros serviços internos ou externos sem validação adequada.

**Exemplo prático:**
Uma API aceita uma URL como entrada e o servidor a acessa sem restrição:

```http
GET /proxy?url=http://localhost:8080/admin
```

Isso permite que um atacante acesse serviços internos protegidos.

**Como evitar:**

* Validar URLs e bloquear IPs internos.
* Usar listas de permissões.
* Executar o serviço com menos privilégios.
