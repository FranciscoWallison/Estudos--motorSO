Entendido. Começarei a tradução do documento "PC Assembly Language" para Markdown. Dada a extensão do livro, irei processá-lo e apresentá-lo em partes.

Vou iniciar com as primeiras páginas, incluindo o prefácio e o início do Capítulo 1.

---

# [cite_start]PC Assembly Language [cite: 1]

[cite_start]Paul A. Carter [cite: 1]

[cite_start]November 16, 2019 [cite: 1]

## Licença

[cite_start]Este trabalho está licenciado sob a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License. [cite: 2] [cite_start]Para ver uma cópia desta licença, visite [http://creativecommons.org/licenses/by-nc-sa/4.0/](http://creativecommons.org/licenses/by-nc-sa/4.0/). [cite: 3]

## Sumário

* [cite_start]Prefácio, V [cite: 5]
* [cite_start]1 Introdução, 1 [cite: 5]
    * [cite_start]1.1 Sistemas Numéricos, 1 [cite: 5]
        * [cite_start]1.1.1 Decimal, 1 [cite: 5]
        * [cite_start]1.1.2 Binário, 1 [cite: 5]
        * [cite_start]1.1.3 Hexadecimal, 2 [cite: 5]
    * [cite_start]1.2 Organização do Computador, 4 [cite: 5]
        * [cite_start]1.2.1 Memória, 4 [cite: 5]
        * [cite_start]1.2.2 A CPU, 5 [cite: 5]
        * [cite_start]1.2.3 A família de CPUs 80x86, 6 [cite: 5]
        * [cite_start]1.2.4 Registradores de 16 bits do 8086, 7 [cite: 5]
        * [cite_start]1.2.5 Registradores de 32 bits do 80386, 8 [cite: 5]
        * [cite_start]1.2.6 Modo Real, 8 [cite: 5]
        * [cite_start]1.2.7 Modo Protegido de 16 bits, 9 [cite: 5]
        * [cite_start]1.2.8 Modo Protegido de 32 bits, 10 [cite: 5]
        * [cite_start]1.2.9 Interrupções, 10 [cite: 5]
    * [cite_start]1.3 Linguagem Assembly, 11 [cite: 5]
        * [cite_start]1.3.1 Linguagem de máquina, 11 [cite: 5]
        * [cite_start]1.3.2 Linguagem Assembly, 11 [cite: 5]
        * [cite_start]1.3.3 Operandos de instrução, 12 [cite: 5]
        * [cite_start]1.3.4 Instruções básicas, 12 [cite: 5]
        * [cite_start]1.3.5 Diretivas, 13 [cite: 5]
        * [cite_start]1.3.6 Entrada e Saída, 16 [cite: 5]
        * [cite_start]1.3.7 Depuração, 16 [cite: 5]
    * [cite_start]1.4 Criando um Programa, 18 [cite: 5]
        * [cite_start]1.4.1 Primeiro programa, 18 [cite: 5]
        * [cite_start]1.4.2 Dependências do compilador, 22 [cite: 5]
        * [cite_start]1.4.3 Montando o código, 22 [cite: 5]
        * [cite_start]1.4.4 Compilando o código C, 23 [cite: 5]
        * [cite_start]1.4.5 Linkando os arquivos objeto, 23 [cite: 5]
        * [cite_start]1.4.6 Entendendo um arquivo de listagem assembly, 23 [cite: 5]
    * [cite_start]1.5 Arquivo Esqueleto, 25 [cite: 8]
* [cite_start]2 Linguagem Assembly Básica, 27 [cite: 8]
    * [cite_start]2.1 Trabalhando com Inteiros, 27 [cite: 8]
        * [cite_start]2.1.1 Representação de inteiros, 27 [cite: 8]
        * [cite_start]2.1.2 Extensão de sinal, 30 [cite: 8]
        * [cite_start]2.1.3 Aritmética de complemento de dois, 33 [cite: 8]
        * [cite_start]2.1.4 Programa de exemplo, 35 [cite: 8]
        * [cite_start]2.1.5 Aritmética de precisão estendida, 36 [cite: 8]
    * [cite_start]2.2 Estruturas de Controle, 37 [cite: 8]
        * [cite_start]2.2.1 Comparações, 37 [cite: 8]
        * [cite_start]2.2.2 Instruções de ramificação, 38 [cite: 8]
        * [cite_start]2.2.3 As instruções de loop, 41 [cite: 8]
    * [cite_start]2.3 Traduzindo Estruturas de Controle Padrão, 42 [cite: 8]
        * [cite_start]2.3.1 Instruções If, 42 [cite: 8]
        * [cite_start]2.3.2 Loops While, 43 [cite: 8]
        * [cite_start]2.3.3 Loops Do while, 43 [cite: 8]
    * [cite_start]2.4 Exemplo: Encontrando Números Primos, 43 [cite: 8]
* [cite_start]3 Operações de Bits, 47 [cite: 8]
    * [cite_start]3.1 Operações de Deslocamento, 47 [cite: 8]
        * [cite_start]3.1.1 Deslocamentos lógicos, 47 [cite: 8]
        * [cite_start]3.1.2 Uso de deslocamentos, 48 [cite: 8]
        * [cite_start]3.1.3 Deslocamentos aritméticos, 48 [cite: 8]
        * [cite_start]3.1.4 Deslocamentos de rotação, 49 [cite: 8]
        * [cite_start]3.1.5 Aplicação simples, 49 [cite: 8]
    * [cite_start]3.2 Operações Bit a Bit Booleanas, 50 [cite: 8]
        * [cite_start]3.2.1 A operação AND, 50 [cite: 8]
        * [cite_start]3.2.2 A operação OR, 50 [cite: 8]
        * [cite_start]3.2.3 A operação XOR, 51 [cite: 8]
        * [cite_start]3.2.4 A operação NOT, 51 [cite: 8]
        * [cite_start]3.2.5 A instrução TEST, 51 [cite: 8]
        * [cite_start]3.2.6 Usos de operações de bits, 52 [cite: 8]
    * [cite_start]3.3 Evitando Ramificações Condicionais, 53 [cite: 8]
    * [cite_start]3.4 Manipulando bits em C, 56 [cite: 8]
        * [cite_start]3.4.1 Os operadores bit a bit de C, 56 [cite: 8]
        * [cite_start]3.4.2 Usando operadores bit a bit em C, 56 [cite: 8]
    * [cite_start]3.5 Representações Big e Little Endian, 57 [cite: 8]
        * [cite_start]3.5.1 Quando se preocupar com Little e Big Endian, 59 [cite: 8]
    * [cite_start]3.6 Contagem de Bits, 60 [cite: 8]
        * [cite_start]3.6.1 Método um, 60 [cite: 8]
        * [cite_start]3.6.2 Método dois, 61 [cite: 8]
        * [cite_start]3.6.3 Método três, 62 [cite: 8]
* [cite_start]4 Subprogramas, 65 [cite: 10]
    * [cite_start]4.1 Endereçamento Indireto, 65 [cite: 10]
    * [cite_start]4.2 Exemplo Simples de Subprograma, 66 [cite: 10]
    * [cite_start]4.3 A Pilha, 68 [cite: 10]
    * [cite_start]4.4 As Instruções CALL e RET, 69 [cite: 10]
    * [cite_start]4.5 Convenções de Chamada, 70 [cite: 10]
        * [cite_start]4.5.1 Passando parâmetros na pilha, 70 [cite: 10]
        * [cite_start]4.5.2 Variáveis locais na pilha, 75 [cite: 10]
    * [cite_start]4.6 Programas Multi-Módulos, 77 [cite: 10]
    * [cite_start]4.7 Interface Assembly com C, 80 [cite: 10]
        * [cite_start]4.7.1 Salvando registradores, 81 [cite: 10]
        * [cite_start]4.7.2 Rótulos de funções, 82 [cite: 10]
        * [cite_start]4.7.3 Passando parâmetros, 82 [cite: 10]
        * [cite_start]4.7.4 Calculando endereços de variáveis locais, 82 [cite: 10]
        * [cite_start]4.7.5 Retornando valores, 83 [cite: 10]
        * [cite_start]4.7.6 Outras convenções de chamada, 83 [cite: 10]
        * [cite_start]4.7.7 Exemplos, 85 [cite: 10]
        * [cite_start]4.7.8 Chamando funções C do assembly, 88 [cite: 10]
    * [cite_start]4.8 Subprogramas Reentrantes e Recursivos, 89 [cite: 10]
        * [cite_start]4.8.1 Subprogramas recursivos, 89 [cite: 10]
        * [cite_start]4.8.2 Revisão dos tipos de armazenamento de variáveis C, 91 [cite: 10]
* [cite_start]5 Arrays, 95 [cite: 10]
    * [cite_start]5.1 Introdução, 95 [cite: 10]
        * [cite_start]5.1.1 Definindo arrays, 95 [cite: 10]
        * [cite_start]5.1.2 Acessando elementos de arrays, 96 [cite: 10]
        * [cite_start]5.1.3 Endereçamento indireto mais avançado, 98 [cite: 10]
        * [cite_start]5.1.4 Exemplo, 99 [cite: 10]
        * [cite_start]5.1.5 Arrays Multidimensionais, 103 [cite: 10]
    * [cite_start]5.2 Instruções de Array/String, 106 [cite: 10]
        * [cite_start]5.2.1 Leitura e escrita de memória, 106 [cite: 10]
        * [cite_start]5.2.2 O prefixo de instrução REP, 108 [cite: 10]
        * [cite_start]5.2.3 Instruções de string de comparação, 109 [cite: 10]
        * [cite_start]5.2.4 Os prefixos de instrução REPx, 109 [cite: 10]
        * [cite_start]5.2.5 Exemplo, 111 [cite: 10]
* [cite_start]6 Ponto Flutuante, 117 [cite: 10]
    * [cite_start]6.1 Representação de Ponto Flutuante, 117 [cite: 10]
        * [cite_start]6.1.1 Números binários não inteiros, 117 [cite: 10]
        * [cite_start]6.1.2 Representação de ponto flutuante IEEE, 119 [cite: 10]
    * [cite_start]6.2 Aritmética de Ponto Flutuante, 122 [cite: 10]
        * [cite_start]6.2.1 Adição, 122 [cite: 10]
        * [cite_start]6.2.2 Subtração, 123 [cite: 10]
        * [cite_start]6.2.3 Multiplicação e divisão, 123 [cite: 13]
        * [cite_start]6.2.4 Consequências para programação, 124 [cite: 13]
    * [cite_start]6.3 O Coprocessador Numérico, 124 [cite: 13]
        * [cite_start]6.3.1 Hardware, 124 [cite: 13]
        * [cite_start]6.3.2 Instruções, 125 [cite: 13]
        * [cite_start]6.3.3 Exemplos, 130 [cite: 13]
        * [cite_start]6.3.4 Fórmula quadrática, 130 [cite: 13]
        * [cite_start]6.3.5 Lendo array de arquivo, 133 [cite: 13]
        * [cite_start]6.3.6 Encontrando primos, 135 [cite: 13]
* [cite_start]7 Estruturas e C++, 143 [cite: 13]
    * [cite_start]7.1 Estruturas, 143 [cite: 13]
        * [cite_start]7.1.1 Introdução, 143 [cite: 13]
        * [cite_start]7.1.2 Alinhamento de memória, 145 [cite: 13]
        * [cite_start]7.1.3 Campos de bits, 146 [cite: 13]
        * [cite_start]7.1.4 Usando estruturas em assembly, 150 [cite: 13]
    * [cite_start]7.2 Assembly e C++, 150 [cite: 13]
        * [cite_start]7.2.1 Sobrecarga e "Name Mangling", 151 [cite: 13]
        * [cite_start]7.2.2 Referências, 153 [cite: 13]
        * [cite_start]7.2.3 Funções inline, 154 [cite: 13]
        * [cite_start]7.2.4 Classes, 156 [cite: 13]
        * [cite_start]7.2.5 Herança e Polimorfismo, 166 [cite: 13]
        * [cite_start]7.2.6 Outras características do C++, 171 [cite: 13]
* [cite_start]A Instruções 80x86, 173 [cite: 13]
    * [cite_start]A.1 Instruções Não-Ponto Flutuante, 173 [cite: 13]
    * [cite_start]A.2 Instruções de Ponto Flutuante, 179 [cite: 13]

---

## Prefácio

### Propósito

[cite_start]O propósito deste livro é dar ao leitor uma melhor compreensão de como os computadores realmente funcionam em um nível mais baixo do que em linguagens de programação como Pascal. [cite: 14] [cite_start]Ao obter uma compreensão mais profunda de como os computadores funcionam, o leitor pode frequentemente ser muito mais produtivo desenvolvendo software em linguagens de nível superior como C e C++. [cite: 15] [cite_start]Aprender a programar em linguagem assembly é uma excelente maneira de atingir esse objetivo. [cite: 15]

[cite_start]Outros livros de linguagem assembly para PC ainda ensinam a programar o processador 8086 que o PC original usava em 1981! [cite: 16] [cite_start]O processador 8086 só suportava o modo real. [cite: 17] [cite_start]Neste modo, qualquer programa pode endereçar qualquer memória ou dispositivo no computador. [cite: 17] [cite_start]Este modo não é adequado para um sistema operacional seguro e multitarefa. [cite: 18] [cite_start]Em vez disso, este livro discute como programar o 80386 e processadores posteriores no modo protegido (o modo em que o Windows e o Linux são executados). [cite: 19] [cite_start]Este modo suporta os recursos que os sistemas operacionais modernos esperam, como memória virtual e proteção de memória. [cite: 20] Existem várias razões para usar o modo protegido:

1. [cite_start]É mais fácil programar no modo protegido do que no modo real do 8086 que outros livros usam. [cite: 21]
2. [cite_start]Todos os sistemas operacionais de PC modernos funcionam no modo protegido. [cite: 22]
3. [cite_start]Há software livre disponível que roda neste modo. [cite: 23]

[cite_start]A falta de livros didáticos para programação assembly de PC em modo protegido é a principal razão pela qual o autor escreveu este livro. [cite: 24]

[cite_start]Como aludido acima, este texto faz uso de software Livre/Open Source: nomeadamente, o montador NASM e o compilador C/C++ DJGPP. [cite: 25] [cite_start]Ambos estão disponíveis para download na Internet. [cite: 26] [cite_start]O texto também discute como usar código assembly NASM sob o sistema operacional Linux e com os compiladores C/C++ da Borland e da Microsoft sob o Windows. [cite: 27] [cite_start]Exemplos para todas essas plataformas podem ser encontrados em meu site: [http://pacman128.github.io/pcasm/](http://pacman128.github.io/pcasm/). [cite: 28] [cite_start]Você deve baixar o código de exemplo se deseja montar e executar muitos dos exemplos deste tutorial. [cite: 29]

[cite_start]Este texto não tenta cobrir todos os aspectos da programação assembly. [cite: 31] [cite_start]O autor tentou cobrir os tópicos mais importantes com os quais todos os programadores deveriam estar familiarizados. [cite: 32]

### Agradecimentos

[cite_start]O autor gostaria de agradecer aos muitos programadores em todo o mundo que contribuíram para o movimento Free/Open Source. [cite: 33] [cite_start]Todos os programas e até este próprio livro foram produzidos usando software livre. [cite: 34] [cite_start]Especificamente, o autor gostaria de agradecer a John S. Fine, Simon Tatham, Julian Hall e outros por desenvolverem o montador NASM no qual todos os exemplos deste livro são baseados; [cite: 35] [cite_start]DJ Delorie por desenvolver o compilador C/C++ DJGPP usado; as inúmeras pessoas que contribuíram para o compilador GNU GCC no qual o DJGPP se baseia; [cite: 36] [cite_start]Donald Knuth e outros por desenvolverem as linguagens de composição tipográfica TEX e LaTeX2e que foram usadas para produzir o livro; [cite: 37] [cite_start]Richard Stallman (fundador da Free Software Foundation), Linus Torvalds (criador do kernel Linux) e outros que produziram o software subjacente que o autor usou para produzir este trabalho. [cite: 38]

[cite_start]Obrigado às seguintes pessoas pelas correções: [cite: 39]

* [cite_start]John S. Fine [cite: 39]
* [cite_start]Marcelo Henrique Pinto de Almeida [cite: 39]
* [cite_start]Sam Hopkins [cite: 39]
* [cite_start]Nick D'Imperio [cite: 39]
* [cite_start]Jeremiah Lawrence [cite: 39]
* [cite_start]Ed Beroset [cite: 39]
* [cite_start]Jerry Gembarowski [cite: 39]
* [cite_start]Ziqiang Peng [cite: 39]
* [cite_start]Eno Compton [cite: 39]
* [cite_start]Josh I Cates [cite: 39]
* [cite_start]Mik Mifflin [cite: 39]
* [cite_start]Luke Wallis [cite: 39]
* [cite_start]Gaku Ueda [cite: 39]
* [cite_start]Brian Heward [cite: 39]
* [cite_start]Chad Gorshing [cite: 40]
* [cite_start]F. Gotti [cite: 40]
* [cite_start]Bob Wilkinson [cite: 40]
* [cite_start]Markus Koegel [cite: 40]
* [cite_start]Louis Taber [cite: 40]
* [cite_start]Dave Kiddell [cite: 40]
* [cite_start]Eduardo Horowitz [cite: 40]
* [cite_start]Sébastien Le Ray [cite: 40]
* [cite_start]Nehal Mistry [cite: 40]
* [cite_start]Jianyue Wang [cite: 40]
* [cite_start]Jeremias Kleer [cite: 40]
* [cite_start]Marc Janicki [cite: 40]
* [cite_start]Trevor Hansen [cite: 40]
* [cite_start]Giacomo Bruschi [cite: 40]
* [cite_start]Leonardo Rodríguez Mújica [cite: 40]
* [cite_start]Ulrich Bicheler [cite: 40]
* [cite_start]Wu Xing [cite: 40]
* [cite_start]Oleksandr Baranyuk [cite: 40]

### Recursos na Internet

| [cite_start]Autor's page       | [http://pacman128.github.io/](http://pacman128.github.io/)       | [cite: 41] |
| :----------------- | :-------------------------------- | :--------- |
| [cite_start]NASM SourceForge page | [http://www.nasm.us/](http://www.nasm.us/)               | [cite: 41] |
| [cite_start]DJGPP              | [http://www.delorie.com/djgpp](http://www.delorie.com/djgpp)      | [cite: 41] |
| [cite_start]The Art of Assembly | [http://webster.cs.ucr.edu/](http://webster.cs.ucr.edu/)        | [cite: 41] |
| [cite_start]USENET             | comp.lang.asm.x86                 | [cite: 41] |

### Feedback

[cite_start]O autor agradece qualquer feedback sobre este trabalho. [cite: 42]

[cite_start]E-mail: pacman128@gmail.com [cite: 42]

[cite_start]WWW: [http://pacman128.github.io/pcasm/](http://pacman128.github.io/pcasm/) [cite: 42]

## Capítulo 1

### 1.1 Sistemas Numéricos

[cite_start]A memória em um computador consiste em números. [cite: 44] [cite_start]A memória do computador não armazena esses números em decimal (base 10). [cite: 45] [cite_start]Como isso simplifica muito o hardware, os computadores armazenam todas as informações em um formato binário (base 2). [cite: 46]

[cite_start]Primeiro, vamos revisar o sistema decimal. [cite: 47]

#### 1.1.1 Decimal

[cite_start]Números de base 10 são compostos por 10 dígitos possíveis (0-9). [cite: 48] [cite_start]Cada dígito de um número tem uma potência de 10 associada a ele com base em sua posição no número. [cite: 49] Por exemplo:

[cite_start]$234=2\times10^{2}+3\times10^{1}+4\times10^{0}$ [cite: 49]

#### 1.1.2 Binário

[cite_start]Números de base 2 são compostos por 2 dígitos possíveis (0 e 1). [cite: 50] [cite_start]Cada dígito de um número tem uma potência de 2 associada a ele com base em sua posição no número. [cite: 51] (Um único dígito binário é chamado de bit.) Por exemplo¹:

[cite_start]$11001_{2}=1\times2^{4}+1\times2^{3}+0\times2^{2}+0\times2^{1}+1\times2^{0}$ [cite: 51]
[cite_start]$=16+8+1$ [cite: 51]
[cite_start]$=25$ [cite: 51]

[cite_start]Isso mostra como o binário pode ser convertido para decimal. [cite: 52] [cite_start]A Tabela 1.1 mostra como os primeiros números são representados em binário. [cite: 52] [cite_start]A Figura 1.1 mostra como dígitos binários individuais (ou seja, bits) são adicionados. [cite: 53] Aqui está um exemplo:

[cite_start]A 2ª subscrição é usada para mostrar que o número é representado em binário, não em decimal[cite: 54].

| Decimal | Binário | Decimal | Binário |
| :------ | :------ | :-------- | :------ |
| 0       | 0000    | 8         | 1000    |
| 1       | 0001    | 9         | 1001    |
| 2       | 0010    | 10        | 1010    |
| 3       | 0011    | 11        | 1011    |
| 4       | 0100    | 12        | 1100    |
| 5       | 0101    | 13        | 1101    |
| 6       | 0110    | 14        | 1110    |
| 7       | 0111    | 15        | 1111    |

[cite_start]Tabela 1.1: Decimal de 0 a 15 em Binário [cite: 57]

|             | Nenhum carry anterior | | | Carry anterior | |
| :---------- | :-------------------- | :- | :-- | :------------- | :-- |
| 0           | 0                     | 1  | 1   | 0              | 0   |
| +0          | +0                    | +1 | +1  | +0             | +1  |
| 0           | 1                     | 1  | 0   | 0              | 1   |
| | C                     | | C   | C              | C   |

[cite_start]Figura 1.1: Adição binária (c significa carry) [cite: 59]

[cite_start]$\begin{aligned}11011_{2}\\ \underline{+10001_{2}}\\ 101100_{2}\end{aligned}$ [cite: 59]

[cite_start]Se considerarmos a seguinte divisão decimal: [cite: 59]

[cite_start]$1234\div10=123~r~4$ [cite: 59]

[cite_start]pode-se ver que essa divisão remove o dígito decimal mais à direita do número e desloca os outros dígitos decimais uma posição para a direita. [cite: 59] [cite_start]Dividir por dois realiza uma operação semelhante, mas para os dígitos binários do número. [cite: 60] Considere a seguinte divisão binária:

[cite_start]$1101_{2}\div10_{2}=110_{2}r~1$ [cite: 61]

[cite_start]Esse fato pode ser usado para converter um número decimal para sua representação binária equivalente, como mostra a Figura 1.2. [cite: 61] [cite_start]Esse método encontra o dígito mais à direita primeiro, esse dígito é chamado de bit menos significativo (lsb). [cite: 62] [cite_start]O dígito mais à esquerda é chamado de bit mais significativo (msb). [cite: 63] [cite_start]A unidade básica de memória consiste em 8 bits e é chamada de byte. [cite: 64]

#### 1.1.3 Hexadecimal

[cite_start]Números hexadecimais usam a base 16. [cite: 65] [cite_start]Hexadecimal (ou hex, para abreviar) pode ser usado como uma abreviação para números binários. [cite: 65] [cite_start]Hex tem 16 dígitos possíveis. [cite: 66] [cite_start]Isso cria um problema, pois não há símbolos para usar para esses dígitos extras depois do 9. [cite: 67] [cite_start]Por convenção, letras são usadas para esses dígitos extras. [cite: 67] [cite_start]Os 16 dígitos hexadecimais são 0-9, depois A, B, C, D, E e F. [cite: 68] [cite_start]O dígito A é equivalente a 10 em decimal, B é 11, etc. [cite: 68] [cite_start]Cada dígito de um número hexadecimal tem uma potência de 16 associada a ele. [cite: 68] Exemplo:

[cite_start]$2BD_{16}=2\times16^{2}+11\times16^{1}+13\times16^{0}$ [cite: 69]
[cite_start]$=512+176+13$ [cite: 69]
[cite_start]$=701$ [cite: 69]

[cite_start]Para converter de decimal para hexadecimal, use a mesma ideia que foi usada para a conversão binária, exceto dividindo por 16. [cite: 69] [cite_start]Veja a Figura 1.3 para um exemplo. [cite: 69] [cite_start]A razão pela qual o hexadecimal é útil é que existe uma maneira muito simples de converter entre hexadecimal e binário. [cite: 70] [cite_start]Números binários ficam grandes e complicados rapidamente. [cite: 71] [cite_start]O hexadecimal fornece uma maneira muito mais compacta de representar o binário. [cite: 71]

[cite_start]Para converter um número hexadecimal para binário, basta converter cada dígito hexadecimal para um número binário de 4 bits. [cite: 72] [cite_start]Por exemplo, $24D_{16}$ é convertido para 0010 0100 11012. [cite: 73]

| Decimal | Binário             |
| :------ | :------------------ |
| 25 ÷ 2  | = 12 r 1 11001 ÷ 10 |
| 12 ÷ 2  | = 6 r 0 1100 ÷ 10   |
| 6 ÷ 2   | = 3 r 0 110 ÷ 10    |
| 3 ÷ 2   | = 1 r 1 11 ÷ 10     |
| 1 ÷ 2   | = 0 r 1 1 ÷ 10      |
|         |                     |

[cite_start]Assim $25_{10}=11001_{2}$ [cite: 67]

[cite_start]Figura 1.2: Conversão Decimal [cite: 67]

| 589 ÷ 16 | = 36 r 13 |
| :------- | :-------- |
| 36 ÷ 16  | = 2 r 4   |
| 2 ÷ 16   | = 0 r 2   |

[cite_start]Assim $589=24D_{16}$ [cite: 69]

[cite_start]Figura 1.3: [cite: 69]

[cite_start]Note que os zeros à esquerda dos 4 bits são importantes! [cite: 76] [cite_start]Se o zero à esquerda para o dígito do meio de $24D_{16}$ não for usado, o resultado estará errado. [cite: 77] [cite_start]A conversão de binário para hexadecimal é tão fácil quanto. [cite: 78] [cite_start]Faz-se o processo inverso. [cite: 78] [cite_start]Converta cada segmento de 4 bits do binário para hexadecimal. [cite: 79] [cite_start]Comece da direita, não da esquerda do número binário. [cite: 79] [cite_start]Isso garante que o processo use os segmentos de 4 bits corretos. [cite: 80] Exemplo:

[cite_start]110 0000 0101 1010 0111 $1110_{2}$ [cite: 81]
[cite_start]6 0 5 A 7 $E_{16}$ [cite: 81]

[cite_start]Um número de 4 bits é chamado de nibble. [cite: 81] [cite_start]Assim, cada dígito hexadecimal corresponde a um nibble. [cite: 82] [cite_start]Dois nibbles formam um byte, e assim um byte pode ser representado por um número hexadecimal de 2 dígitos. [cite: 82] [cite_start]O valor de um byte varia de 0 a 11111111 em binário, 0 a FF em hexadecimal e 0 a 255 em decimal. [cite: 83]

#### 1.2 Organização do Computador

##### 1.2.1 Memória

[cite_start]A memória é medida em unidades de kilobytes ($2^{10} = 1.024$ bytes), megabytes ($2^{20} = 1.048.576$ bytes) e gigabytes ($2^{30} = 1.073.741.824$ bytes). [cite: 85] A unidade básica de memória é um byte. [cite_start]Um computador com 32 megabytes de memória pode armazenar aproximadamente 32 milhões de bytes de informação. [cite: 85] [cite_start]Cada byte na memória é rotulado por um número único conhecido como seu endereço, como mostra a Figura 1.4. [cite: 85]

| Endereço | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   |
| :------- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| Memória  | 2A  | 45  | B8  | 20  | 8F  | CD  | 12  | 2E  |

[cite_start]Figura 1.4: Endereços de Memória [cite: 87]

[cite_start]Frequentemente, a memória é usada em pedaços maiores do que bytes únicos. [cite: 87] [cite_start]Na arquitetura de PC, nomes foram dados a essas seções maiores de memória, como mostra a Tabela 1.2. [cite: 88]

| word        | [cite_start]2 bytes  | [cite: 75] |
| :---------- | :------- | :--------- |
| double word | [cite_start]4 bytes  | [cite: 75] |
| quad word   | [cite_start]8 bytes  | [cite: 75] |
| paragraph   | [cite_start]16 bytes | [cite: 75] |

[cite_start]Tabela 1.2: Unidades de Memória [cite: 76]

[cite_start]Todos os dados na memória são numéricos. [cite: 90] [cite_start]Caracteres são armazenados usando um código de caractere que mapeia números para caracteres. [cite: 90] [cite_start]Um dos códigos de caractere mais comuns é conhecido como ASCII (American Standard Code for Information Interchange). [cite: 91] [cite_start]Um novo código, mais completo, que está suplantando o ASCII é o Unicode. [cite: 92] [cite_start]Uma diferença fundamental entre os dois códigos é que o ASCII usa um byte para codificar um caractere, mas o Unicode usa vários bytes. [cite: 93] [cite_start]Existem várias formas diferentes de Unicode. [cite: 94] [cite_start]Em compiladores C/C++ x86, o Unicode é representado em código usando o tipo `wchar_t` e a codificação UTF-16, que usa 16 bits (ou uma palavra) por caractere. [cite: 94] Por exemplo, ASCII mapeia o byte `41_16` ($65_{10}$) para o caractere "A" maiúsculo; [cite_start]UTF-16 o mapeia para a palavra `0041_16`. [cite: 95] [cite_start]Como ASCII usa um byte, ele é limitado a apenas 256 caracteres diferentes³. [cite: 95] [cite_start]Unicode estende os valores ASCII e permite que muitos mais caracteres sejam representados. [cite: 95] [cite_start]Isso é importante para representar caracteres de todas as linguagens do mundo. [cite: 95]

##### 1.2.2 A CPU

[cite_start]A Unidade Central de Processamento (CPU) é o dispositivo físico que executa as instruções. [cite: 95] [cite_start]As instruções que as CPUs executam são geralmente muito simples. [cite: 95] [cite_start]As instruções podem exigir que os dados sobre os quais atuam estejam em locais de armazenamento especiais na própria CPU, chamados registradores. [cite: 96] [cite_start]A CPU pode acessar dados em registradores muito mais rápido do que dados na memória. [cite: 96] [cite_start]No entanto, o número de registradores em uma CPU é limitado, então o programador deve ter cuidado para manter apenas os dados atualmente usados nos registradores. [cite: 96]

[cite_start]As instruções que um tipo de CPU executa compõem a linguagem de máquina da CPU. [cite: 96] [cite_start]Programas de máquina têm uma estrutura muito mais básica do que linguagens de nível superior. [cite: 97] [cite_start]As instruções da linguagem de máquina são codificadas como números brutos, não em formatos de texto amigáveis. [cite: 97] [cite_start]Uma CPU deve ser capaz de decodificar o propósito de uma instrução muito rapidamente para funcionar eficientemente. [cite: 98] [cite_start]A linguagem de máquina é projetada com esse objetivo em mente, não para ser facilmente decifrada por humanos. [cite: 99] [cite_start]Programas escritos em outras linguagens devem ser convertidos para a linguagem de máquina nativa da CPU para serem executados no computador. [cite: 100] [cite_start]Um compilador é um programa que traduz programas escritos em uma linguagem de programação para a linguagem de máquina de uma arquitetura de computador específica. [cite: 101] [cite_start]Em geral, cada tipo de CPU tem sua própria linguagem de máquina única. [cite: 102] [cite_start]Essa é uma das razões pelas quais programas escritos para um Mac não podem ser executados em um PC tipo IBM. [cite: 103]

[cite_start]Computadores usam um relógio para sincronizar a execução das instruções. [cite: 104] [cite_start]O relógio pulsa em uma frequência fixa (conhecida como velocidade de clock). [cite: 105] [cite_start]Quando você compra um computador de 1,5 GHz, 1,5 GHz é a frequência desse relógio. [cite: 106] [cite_start]O relógio não controla minutos e segundos. [cite: 107] [cite_start]Ele simplesmente bate em uma taxa constante. [cite: 108] [cite_start]A eletrônica da CPU usa os pulsos para realizar suas operações corretamente, assim como as batidas de um metrônomo ajudam a tocar música no ritmo correto. [cite: 112] [cite_start]O número de batidas (ou, como são geralmente chamados, ciclos) que uma instrução requer depende da geração e do modelo da CPU. [cite: 113] [cite_start]O número de ciclos depende das instruções anteriores e de outros fatores também. [cite: 114]

[cite_start]¹Na verdade, ASCII usa apenas os 7 bits inferiores e, portanto, tem apenas 128 valores diferentes para usar. [cite: 109]
[cite_start]²Na verdade, os pulsos do clock são usados em muitos componentes diferentes de um computador. [cite: 109] [cite_start]Os outros componentes frequentemente usam velocidades de clock diferentes das da CPU. [cite: 110]
³GHz significa gigahertz ou um bilhão de ciclos por segundo. [cite_start]Uma CPU de 1,5 GHz tem 1,5 bilhão de pulsos de clock por segundo. [cite: 111]
