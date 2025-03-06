
## Engenharia reversa e hacking de memória com Cheat Engine

O que estamos fazendo?

Vamos modificar instruções na memória do programa.

A ideia é que, ao clicar em um botão (HIT ME)  no programa, ao invés de diminuir um valor, ele aumente.

Mas, antes de fazermos essa alteração, precisamos encontrar o endereço onde esse valor está armazenado. 

Primeir passo é ir no modo de treino do *CE*

![alt text](image.png)

E dar ok ou next até a etapa chegar na 7 ou (Step 7)

![alt text](image-1.png)
nessa etaá tem o botão que precisaremos modificar o seu evento.

Selecione o processo que iremos trabalhar, que nesse caso é o tutorial.
![alt text](image-2.png)

Iremos sacaner o valor, que no meu caso é *99* até achar o endereço de memoria.
![alt text](image-3.png)

No meu caso o endereço é *0176B600*
![alt text](image-4.png)

Apos seleciona a memoria precisaremos ir para o modo de depuração/debugger do CE
![alt text](image-5.png)

Ao clicar em (HIT ME) irá aprensetar o evento em tela sobre o endereço que estamos observado, que no meu caso *0176B600*
![alt text](image-6.png)