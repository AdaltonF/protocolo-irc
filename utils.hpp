// Adalton de Sena Almeida Filho - 12542435
// Rafael Zimmer - 12542612
#ifndef UTILS_HPP
#define UTILS_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <regex>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#define MSG_SIZE 2048                                    // Limite de caracteres em uma única mensagem
#define NICK_SIZE 50                                     // Limite de caracteres de um apelido
#define MAX_RET 5                                        // Número máximo de retransmissões por cliente
#define RGX_CMD "^\\s*/(\\w+) *(\\#?[\\+\\-]?[\\w\\.]*)" // Regex para detectar comandos dados por usuários
#define NICK_MIN 3                                       // Número mínimo de caracteres para um apelido
#define NICK_MAX 50                                      // Número máximo de caracteres para um apelido

using server_data = std::pair<std::string, uint16_t>;  // IP e porta
using server_dns = std::map<std::string, server_data>; // Mapeia um nome ao servidor

/*
 *   Verifica erros. Se existerem, eles são printado e o programa é encerrado.
 *
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is encountered.
 */
void check_error(int status, int error_num, const char *msg);

/*
 *   Quebra a mensagem em pedaços com no máximo, MSG_SIZE+1 chars (incluindo '\0').
 *
 *   Parameters:
 *       msg (string): message to be broken in smaller parts (if possible).
 *   Returns:
 *       vector of strings: vector containing all chunks of the
 *   partitionated message.
 */
std::vector<std::string> break_msg(std::string msg);

/*
 *   Obtem as tuplas do arquivo dns, que tenta reproduzir o funcionamento de um servidor dns.
 *
 *   Return:
 *       DNS(server_dns): map with all the tuples from the file.
 */
server_dns get_dns();

#endif