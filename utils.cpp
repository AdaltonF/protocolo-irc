// Adalton de Sena Almeida Filho - 12542435
// Rafael Zimmer - 12542612
#include "utils.hpp"

/*
 *   Verifica erros. Se existerem, eles são printado e o programa é encerrado.
 *
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is encountered.
 */
void check_error(int status, int error_num, const char *msg) {
    if (status == error_num) {
        perror(msg);
    }
}

/*
 *   Quebra a mensagem em pedaços com no máximo, MSG_SIZE+1 chars (incluindo '\0').
 *
 *   Parameters:
 *       msg (string): message to be broken in smaller parts (if possible).
 *   Returns:
 *       vector<char[MSG_SIZE+1]>: vector containing all chunks of the
 *   partitionated message.
 */
std::vector<std::string> break_msg(std::string msg) {

    int n_chunks = int(ceil((double)msg.size() / MSG_SIZE));
    std::vector<std::string> chunks(n_chunks);

    int j, start_submsg = 0;
    for (int i = 0; i < n_chunks; i++) {
        for (j = start_submsg; j < (start_submsg + MSG_SIZE); j++) {
            chunks[i] += msg[j];
        }
        chunks[i] += '\0';
        start_submsg = j;
    }

    return chunks;
}

/*
 *   Obtem as tuplas do arquivo dns, que tenta reproduzir o funcionamento de um servidor dns.
 *
 *   Return:
 *       DNS(server_dns): map with all the tuples from the file.
 *
 *   OBS: The definition of the struct server_dns is on utils.hpp
 */
server_dns get_dns() {
    std::ifstream reader("dns.txt", std::ifstream::in);
    std::string name;
    std::string ip;
    uint16_t port;

    server_dns DNS;

    while (reader.good()) {
        reader >> name;
        reader >> ip;
        reader >> port;

        DNS.insert(make_pair(name, make_pair(ip, port)));
    }

    reader.close();
    return DNS;
}
