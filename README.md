# Implementação do Protocolo IRC

Projeto feito para a disciplina SSC0142 - Redes de Computadores, ministrada pela Prof.ª Kalinka Branco do ICMC-USP

### Alunos:
- Adalton de Sena Almeida Filho - 12542435
- Rafael Zimmer - 12542612

## Requisitos

## Instruções

## Casos de Teste

## Resultados Esperados

## Comandos

- `/join` nomeCanal - Entra no canal;
- `/nickname` apelidoDesejado - O cliente passa a ser reconhecido pelo apelido especificado;
- `/ping` - O servidor retorna "pong"assim que receber a mensagem.  

**Comandos apenas para administradores de canais:**
  
- `/kick` nomeUsurio - Fecha a conexão de um usuário especificado
- `/mute` nomeUsurio - Faz com que um usuário não possa enviar mensagens neste canal
- `/unmute` nomeUsurio - Retira o mute de um usuário.
- `/whois` nomeUsurio - Retorna o endereço IP do usuário apenas para o administrador

## Referência

O código foi livremente inspirado na implementação presente neste [repositório](https://github.com/vitor-san/irc-redes). Agradecemos 
aos autores pelo bom trabalho desenvolvido.
