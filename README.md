# Implementação do Protocolo IRC

Projeto feito por alunos da disciplina SSC0142 - Redes de Computadores, ministrada pela professora Kalinka Branco. ICMC-USP

- Adalton de Sena Almeida Filho - 12542435
- Rafael Zimmer - 12542612

# Comandos

- `/join` nomeCanal - Entra no canal;
- `/nickname` apelidoDesejado - O cliente passa a ser reconhecido pelo apelido especificado;
- `/ping` - O servidor retorna "pong"assim que receber a mensagem.
  Comandos apenas para administradores de canais:
- `/kick` nomeUsurio - Fecha a conexão de um usuário especificado
- `/mute` nomeUsurio - Faz com que um usuário não possa enviar mensagens neste canal
- `/unmute` nomeUsurio - Retira o mute de um usuário.
- `/whois` nomeUsurio - Retorna o endereço IP do usuário apenas para o administrador

# Referência

O código foi livremente inspirado na implementação deste [repositório](https://github.com/vitor-san/irc-redes).
