#include "socket.hpp"
#include "utils.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <thread>

using namespace std;

string nickname;

bool running = true;

void handleCtrlC(int signum) { cout << "\nPor favor, use o comando /quit.\n"; }

/*
    Thread para enviar mensagens ao servidor
*/
void send_message(Socket *s) {
    string buffer, cmd;
    string quit_msg("/quit");
    regex r(RGX_CMD); // RGX_CMD definido em "utils.hpp"
    smatch m;
    vector<string> chunks;
    bool success = false;

    // Primeiro, o cliente envia seu apelido ao servidor
    string nick_cmd = "/nickname " + nickname;
    s->send_message(nick_cmd);

    while (running) {
        // Pega a mensagem do cliente e armazena-a em um buffer
        if (getline(cin, buffer)) {
            // Busca comandos na mensagem
            regex_search(buffer, m, r);
            cmd = m[1].str(); // Obtem o primeiro comando, caso exista
            // Se algum comando for encontrado (seguindo as regras de RGX_CMD), então ele é executado
            if (cmd != "") {
                if (cmd == "quit") {
                    running = false;
                } else if (cmd == "nickname") {
                    nickname = m[2].str();
                }

                success = s->send_message(buffer);
                while (!success) {
                    success = s->send_message(buffer);
                }

            } else {
                // Mensagem comum
                chunks = break_msg(buffer);
                for (int i = 0; i < (int)chunks.size(); i++) {
                    // Envia a mensagem ao servidor
                    success = s->send_message(chunks[i]);
                    while (!success) {
                        success = s->send_message(buffer);
                    }
                }
            }
        } else {
            // Algo ocorreun enquanto lia a stdin (EOF ou outro erro)
            running = false;
            success = s->send_message(quit_msg);
            while (!success) {
                // O servidor deve receber o comando /quit, pelo contrário
                // ele não será excluído da hash table do cliente.
                success = s->send_message(quit_msg);
            }
        }
    }
}

/*
    Thread para receber mensagens ao servidor
*/
void receive_message(Socket *s) {
    string buffer;
    int bytes_read = 0;

    while (running) {
        bytes_read = s->receive_message(buffer);

        if (bytes_read <= 0) {
            throw("Ocorreu um erro enquanto lia mensagens do servidor.");
            running = false;
        }

        cout << buffer << endl;
    }
}

/*
    Função utilizada para lista todos os servers do nosso dns
*/
void list_servers(server_dns &DNS) {
    cout << endl;
    for (auto it = DNS.begin(); it != DNS.end(); it++) {
        cout << it->first << endl;
    }
}

/*
 * Função utilizada para popular o IP e a porta do servidor.
 * Retorna o sucesso ou falha da conexão.
 */
bool connect_to(server_dns &DNS, string &server_name, string &ip, uint16_t &port) {
    if ((int)server_name.size() < 5 || (int)server_name.size() > 50)
        return false;
    // Verifica se o servidor está cadastrado no nosso DNS
    if (DNS.count(server_name)) {
        server_data value = DNS[server_name];
        ip = value.first;
        port = value.second;
        return true;
    }
    return false;
}

int main(int argc, const char **argv) {
    string server_name, server_ip, cmd;
    uint16_t server_port;
    regex r(RGX_CMD); // RGX_CMD definido em "utils.hpp"
    smatch m;
    char quit = ' ';
    bool is_in_table, connected = false, has_initial_nick = false;
    server_dns DNS = get_dns();
    Socket *my_socket;
 
    cout << "Seja bem-vindo(a) ao IRC.\n\n";

    if (!has_initial_nick) {
        cout << "Primeiro, crie seu apelido (você poderá alterá-lo mais tarde):\n";
        cin >> nickname;

        while (nickname.size() < NICK_MIN || nickname.size() > NICK_MAX) {
            cout << "\nVocê precisa fornecer um apelido com no mínimo " << NICK_MIN << " e no máximo " << NICK_MAX
                 << " caracteres:\n";
            cin >> nickname;
        }

        getchar();
    }

    while (running) {
        cout << "\nConecte-se a um dos nossos servidores utilizando o comando /connect.\n"
             << "Você pode usar o comando /list para listar todos os servidores disponíveis.\n\n";
        getline(cin, cmd);
        // Filtra os comandos
        regex_search(cmd, m, r);
        // Obtem o comando, caso dado
        cmd = m[1].str();
        if (cmd == "connect") {
            server_name = m[2].str();
            // Obtem o endereço IP e a porta, por meio da tabela DNS
            is_in_table = connect_to(DNS, server_name, server_ip, server_port);
            if (!is_in_table) {
                cout << "Não foi possível encontrar o servidor solicitado na nossa tabela DNS.\n";
                continue;
            }
        } else if (cmd == "list") {
            list_servers(DNS);
            continue;
        } else {
            cout << "Por favor, forneça um comando válido para iniciar o IRC.\n";
            continue;
        }
        // Cria socket
        my_socket = new Socket(server_ip, server_port);
        // Tenta se conectar ao socket (um servidor)
        connected = my_socket->connect_to_address();

        if (connected) {
            // Lidando com SIGINT como solicitado
            signal(SIGINT, handleCtrlC);
            // Cria duas threads, uma para recepção e outra para o envio de mensagens.
            thread send_t(send_message, my_socket);
            thread receive_t(receive_message, my_socket);
            // Aguarda até ambas as threads terminarem
            send_t.join();
            receive_t.join();

        } else {
            while (quit != 's' && quit != 'n') {
                cout << "Você deseja sair? (s/n)" << endl;
                cin >> quit;
            }
            if (quit == 's') {
                delete my_socket;
                exit(EXIT_SUCCESS);
            } else {
                quit = ' ';
            }
        }

        if (!running) {
            cout << "Encerrando a conexão.\n\n\n" << endl;
        }

        signal(SIGINT, SIG_DFL);
        delete my_socket;
    }

    return 0;
}