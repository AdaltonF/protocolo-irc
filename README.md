# Implementação do Protocolo IRC

Projeto feito para a disciplina SSC0142 - Redes de Computadores, ministrada pela Prof.ª Kalinka Branco do ICMC-USP.

A implementação feita é uma adaptação das especificações dadas pelo [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459), que define o 
IRC - *Internet Relay Chat*. Seguindo a especificação do trabalho, os três módulos e o item bônus foram implementados com sucesso. Entretanto, 
a implementação foi feita apenas para o caso em que clientes e servidor estão na mesma máquina. 

[Link para o vídeo explicativo](https://youtu.be/NLXKtKNuHRI).

**Alunos:**
- Adalton de Sena Almeida Filho - 12542435
- Rafael Zimmer - 12542612

## Requisitos

* `gcc 7.5.0` ou versão posterior.
* Versão do C++: `C++11`.

## Instruções

Para rodar o código, é necessário compilá-lo por meio do comando `make all`. A partir disso, obtemos os arquivo `server` e `client`,
que serão utilizados para rodar, respectivamente, o servidor e seus clientes.

Em seguida, execute o comando `./server` para iniciar o servidor. Por padrão, o servidor está definido na porta 9001, que pode ser alterada no
 código `server.cpp`.
 
 Em outro terminal, o cliente pode se conectar ao servidor por meio do comando `./client` e em seguida usar o comando 
 `./connect local`. Para novos cliente, basta abrir outros terminais e fazer os mesmos comandos, atribuindo apelidos diferentes. 

## Comandos
- `/connect local` - Conecta o cliente ao servidor local;
- `/join` nomeCanal - Entra no canal;
- `/nickname` apelidoDesejado - O cliente passa a ser reconhecido pelo apelido especificado;
- `/ping` - O servidor retorna "pong"assim que receber a mensagem.  

**Comandos apenas para administradores de canais:**
  
- `/kick` nomeUsuário - Fecha a conexão de um usuário especificado
- `/mute` nomeUsuário - Faz com que um usuário não possa enviar mensagens neste canal
- `/unmute` nomeUsuário - Retira o mute de um usuário.
- `/whois` nomeUsuário - Retorna o endereço IP do usuário apenas para o administrador. Na nossa implementação, o endereço IP retornado sempre será o *127.0.0.1* (loopback).
- `/mode` *+i* ou *-i* - Altera o modo de funcionamento do canal +i (invite-only) ou -i (público)

## Referência

O código foi livremente inspirado na implementação presente neste [repositório](https://github.com/vitor-san/irc-redes). Agradecemos 
aos autores pelo bom trabalho desenvolvido.
