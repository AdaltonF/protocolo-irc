// Adalton de Sena Almeida Filho - 12542435
// Rafael Zimmer - 12542612
#include "socket.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace std;

#define PORT 9001 // Porta fixa
#define MAX_CONN 10
#define MAX_CHANNEL_LEN 200

// Definição de status dos clientes
#define nick(tup) get<0>(tup)
#define alive(tup) get<1>(tup)
#define allowed(tup) get<2>(tup)
#define muted(tup) get<3>(tup)

using hash_value = tuple<string, bool, bool, bool>; // Armazena os status dos cliente
using client_hash = unordered_map<Socket *, hash_value>;

struct msg_info {
    Socket *sender;
    string channel_name; // Utilizada quando há uma notificação do canal a ser enviada por broadcasting
    string content;
};

struct Channel {
    client_hash members; // Mapeia os cliente do canal
    Socket *admin;
    bool isInviteOnly = false;
    set<string> invited_users; // Armazena o conj. de usuários convidados pelo adm.
};

class Server {
  private:
    // Atributos da Classe Server
    Socket listener;
    map<string, Socket *> users;
    unordered_map<Socket *, string> which_channel;
    unordered_map<string, Channel> channels; // Nome dos canais, começando com '#'
    queue<msg_info> msg_queue;               // Fila das mensagens enviadas em broadcast
    mutex mtx;                               // Controle de zonas críticas

    // Métodos
    bool set_nickname(Socket *client, hash_value &client_tup, string &new_nick);
    void set_alive(hash_value &cli, bool is_alive);
    string assert_nickname(Socket *client);
    bool is_valid_nickname(string &nick, Socket *client);
    string prepare_msg(string &chunk, Socket *client);
    bool send_chunk(string chunk, Socket *client);
    void send_to_queue(msg_info pack);
    void remove_from_channel(Socket *client);
    bool change_channel(Socket *client, string new_channel);
    void channel_notification(string c_name, string notification);

  public:
    // Métodos
    Server(int port); // Construtor da classe Server

    void broadcast();
    void accept();
    void receive(Socket *client);

    // Declaração das threads que executaram as funções acima

    thread broadcast_thread() {
        return thread([=] { broadcast(); });
    }

    thread accept_thread() {
        return thread([=] { accept(); });
    }

    thread receive_thread(Socket *client) {
        return thread([=] { receive(client); });
    }
};

/* ---------------------------- Métodos Privados ----------------------------- */

bool Server::is_valid_nickname(string &nick, Socket *client) {
    // Verifica o tamanho do apelido
    if (nick.size() < NICK_MIN || nick.size() > NICK_MAX) {
        // Retorna o erro ao cliente
        this->send_chunk("Você precisa fornecer um apelido com 5 até 50 caracteres.", client);
        return false;
    }
    // Verifica se o apelido está em uso
    if (this->users.find(nick) != this->users.end()) {
        // Retorna o erro ao cliente
        this->send_chunk("Apelido já está em uso!", client);
        return false;
    }
    return true;
}

/*
 *  Define o apelido do cliente fazendo verificações de uso e tamanho
 *
 *  Parameters:
 *      client(Socket*): The socket of the client
 *      client_tup(hash_value): Tuple of the client
 *      new_nick(string): The new nickname of the client
 */
bool Server::set_nickname(Socket *client, hash_value &client_tup, string &new_nick) {
    // Validação
    if (!is_valid_nickname(new_nick, client))
        return false;

    // Notifica todos os clientes sobre a alteração
    string my_channel = this->which_channel[client];
    this->channel_notification(my_channel, "Usuário " + nick(client_tup) + " mudou seu apelido para " + new_nick + ".");

    // Altera o apelido
    this->users.erase(nick(client_tup));
    this->users.insert(make_pair(new_nick, client));
    this->channels[my_channel].invited_users.erase(nick(client_tup));
    this->channels[my_channel].invited_users.insert(new_nick);
    nick(client_tup) = new_nick;

    return true;
}

/*
 *  Modifica o status 'alive' do cliente passado na função
 *
 *  Parameters:
 *      hash_value& cli: tuple of client to modify his value alive
 *      bool is_alive: new value of alive
 */
void Server::set_alive(hash_value &cli, bool is_alive) {
    // Previne conflitos
    this->mtx.lock();
    alive(cli) = is_alive;
    this->mtx.unlock();
}


string Server::assert_nickname(Socket *client) {
    string buffer, cmd, nick;
    regex r(RGX_CMD); // RGX_CMD definido em "utils.hpp"
    smatch m;
    bool valid = false;

    // Permanece no loop até que um apelido válido seja atribuído
    while (!valid) {
        client->receive_message(buffer);
        // Busca comandos na mensagem
        regex_search(buffer, m, r);
        // Obtém o primeiro comando dado
        cmd = m[1].str();
        if (cmd != "nickname") {
            this->send_chunk("Por favor, forneça o seu apelido após o comando /nickname.", client);
            continue;
        }
        nick = m[2].str();
        valid = this->is_valid_nickname(nick, client);
    }
    // Redistra o apelido
    this->users.insert(make_pair(nick, client));
    // Retorna o apelido
    return nick;
}

/*
 *  Adiciona o apelido do cliente a mensagem
 *
 *  Parameters:
 *      chunk(string): the message
 *      client(Socket *): The socket of the client
 *  Returns:
 *      msg: the string with nickname + message
 */

string Server::prepare_msg(string &chunk, Socket *client) {
    string cur_channel = this->which_channel[client];
    string msg = nick(this->channels[cur_channel].members[client]) + ": " + chunk;
    return msg;
}

/*
 *  Tenta enviar o "chunk da mensagem" ao cliente
 *  Retorna falso em caso de erro.
 */
bool Server::send_chunk(string chunk, Socket *client) {
    bool success = false;
    int status;
    string buffer, cmd;
    regex r(RGX_CMD); // RGX_CMD definido em "utils.hpp"
    smatch m;

    for (int t = 0; t < MAX_RET; t++) {
        success = client->send_message(chunk);
        if (success) {
            return true;
        }
    }

    string cur_channel = this->which_channel[client];
    cout << "Falha ao entregar a mensagem a " << nick(this->channels[cur_channel].members[client])
         << ". Desconectando..." << endl;
    return false;
}

/*
 *  Adiciona a mensagem a fila
 *  Parameters:
 *      msg_info& pack: the message to be pushed
 */
void Server::send_to_queue(msg_info pack) {
    // Previne conflitos
    this->mtx.lock();
    this->msg_queue.push(pack);
    this->mtx.unlock();
}

void Server::remove_from_channel(Socket *client) {

    string my_channel = this->which_channel[client];
    string my_nick = nick(this->channels[my_channel].members[client]);

    // Remove o cliente do canal
    if (this->channels[my_channel].isInviteOnly) {
        this->channels[my_channel].invited_users.erase(my_nick);
    }
    this->channels[my_channel].members.erase(client);

    int members_on_channel = this->channels[my_channel].members.size();

    // Se não há membros no canal, ele é deletado
    if (members_on_channel == 0 && my_channel != "#general") {
        this->channels.erase(my_channel);
    }

    // Se o cliente é adm, define-se o próximo adm.
    else if (this->channels[my_channel].admin == client) {
        Socket *new_adm = this->channels[my_channel].members.begin()->first;
        this->channels[my_channel].admin = new_adm;
        this->send_chunk("Você é o novo administrador do canal!", new_adm);
    }
}

bool Server::change_channel(Socket *client, string new_channel) {
    if (new_channel[0] != '#') {
        // O nome do canal não foi fornecido no formato correto
        this->send_chunk("O nome do canal deve ser precedido por '#'. Ex.: /join #test", client);
        return false;
    }

    mtx.lock();

    string my_channel = this->which_channel[client];
    hash_value myself = this->channels[my_channel].members[client];
    muted(myself) = false;

    // Verifica se o canal é 'invite-only e se o usuário foi convidado
    if (this->channels.find(new_channel) != this->channels.end()) { // Se o canal existe
        if (this->channels[new_channel].isInviteOnly) {
            if (this->channels[new_channel].invited_users.find(nick(myself)) ==
                this->channels[new_channel].invited_users.end()) {
                // Usuário não foi convidado
                this->send_chunk("Você não foi convidado ao canal " + new_channel, client);
                mtx.unlock();
                return false;
            }
        }
    }

    // Deleta o usuário do canal anterior
    this->remove_from_channel(client);

    // Se o novo canal não existe
    if (this->channels.find(new_channel) == this->channels.end()) {
        // Server log
        cout << "Não foi encontrado o canal " << new_channel << ", estou criando-o." << endl;
        // Cria o canal e define o criador como adm
        Channel c;
        c.admin = client;
        c.members.insert(make_pair(client, myself));
        this->channels[new_channel] = c;
    } else {
        this->channels[new_channel].members.insert(make_pair(client, myself));
    }
    this->which_channel[client] = new_channel;

    mtx.unlock();
    return true;
}

void Server::channel_notification(string c_name, string notification) {

    if ((int)notification.size() == 0)
        return;

    msg_info msg;
    msg.channel_name = c_name;
    msg.sender = nullptr;
    msg.content = notification;
    this->send_to_queue(msg);
}

/* ---------------------------- Métodos Públicos ------------------------------ */

// Cria um socket apenas para escutar tentativas de conexão
Server::Server(int port) : listener("any", port) {
    bool success = false;
    // Abre o servido para no máximo MAX_CONN conexões
    success = this->listener.listening(MAX_CONN);
    if (!success) {
        throw("Porta já está em uso.");
    }
    // Cria o canal #general
    Channel gen;
    gen.admin = nullptr;
    this->channels.insert(make_pair("#general", gen));
}

/*
 *  Método para lidar com mensagens recebidas de um cliente específico (socket)
 *
 *  Parameters:
 *      client(Socket*): The socket of the client
 */
void Server::receive(Socket *client) {

    string buffer, cmd, new_nick, new_channel, target_user, mode_args;
    regex r(RGX_CMD); // RGX_CMD definido em "utils.hpp"
    smatch m;
    msg_info msg_pack;
    int status;

    // Verifica o apelido do cliente
    string my_nick = this->assert_nickname(client);


    // Introduz o cliente ao canal geral
    string my_channel = "#general";

    // Registra o cliente e define os valores booleano da hash table:
    // - true para 'alive', true para 'allowed' e false para 'muted'.
    this->channels[my_channel].members.insert(make_pair(client, make_tuple(my_nick, true, true, false)));
    this->which_channel[client] = my_channel;

    hash_value *myself = &(this->channels[my_channel].members[client]);

    // Log
    cout << "Cliente inserido: " << my_nick << "\n";

    this->send_chunk("\nSeja bem-vindo(a) ao nosso servidor!\n", client);

    msg_pack.content = my_nick + " entrou no chat!";
    msg_pack.sender = client;
    this->send_to_queue(msg_pack);

    // Loop até a morte do cliente
    while (alive(*myself)) {
        // Recebe a próxima mensagem
        status = client->receive_message(buffer);
        if (status <= 0) {
            // Ocorreu um erro
            break;
        }
        // Procura comando na mensagem
        regex_search(buffer, m, r);
        // Separa o primeiro comando
        cmd = m[1].str();
        // Se algum comando foi encontrado (seguino as regras doe RGX_CMD), então ele é executado
        if (cmd != "") {
            if (cmd == "quit") {
                this->remove_from_channel(client);
                // Apaga o apelido do cliente do servidor
                this->users.erase(my_nick);
                set_alive(*myself, false);
                this->send_chunk("Você saiu do servidor...", client);
                cout << "Cliente " << my_nick << " saiu" << endl; // Log
            } else if (cmd == "ping") {
                // Envia "pong" ao cliente
                set_alive(*myself, this->send_chunk("pong", client));
                // Log
                if (alive(*myself)) {
                    cout << "Pong enviado ao cliente " << my_nick << endl;
                }
            } else if (cmd == "nickname") {
                // Separa o apelido da mensagem
                new_nick = m[2].str();
                this->set_nickname(client, *myself, new_nick);
                my_nick = new_nick;
            } else if (cmd == "join") {
                // Obtém o nome do canal
                new_channel = m[2].str();
                int new_c_len = (int)new_channel.size();
                if (new_c_len < 1 || new_c_len > MAX_CHANNEL_LEN) {
                    this->send_chunk("O nome do canal deve ter entre 1 a " + to_string(MAX_CHANNEL_LEN) +
                                         " caracteres.",
                                     client);
                }
                // Tenta se juntar ao mesmo canal
                else if (!new_channel.compare(my_channel)) {
                    this->send_chunk("Você já está neste canal.", client);
                }
                // Sucesso
                else {
                    if (this->change_channel(client, new_channel)) {
                        // Operação bem-sucedida
                        this->send_chunk("Você mudou do canal " + my_channel + " para o canal " + new_channel, client);
                        channel_notification(my_channel, my_nick + " saiu do canal.");
                        channel_notification(new_channel, my_nick + " entrou no canal!");
                    }
                }
            }
            // Comandos de administrador
            else if (client == this->channels[my_channel].admin) {
                if (cmd == "mode") {
                    mode_args = m[2].str();
                    if (mode_args == "+i") {
                        this->channels[my_channel].isInviteOnly = true;
                        this->channel_notification(my_channel, "Agora, o canal é invite-only!");
                    } else if (mode_args == "-i") {
                        this->channels[my_channel].isInviteOnly = false;
                        this->channel_notification(my_channel, "Agora, o canal é publico!");
                    } else {
                        this->send_chunk("Forma de uso: /mode (+|-)i", client);
                        if (mode_args[0] != '+' && mode_args[0] != '-') {
                            this->send_chunk("Você pode apenas adicionar (+) ou deletar (-) um modo de um canal.", client);
                        }
                        if (mode_args[1] != 'i') {
                            this->send_chunk("O único modo que você pode utilizar nesse canal é 'i' (invite-only)",
                                             client);
                        }
                    }
                } else {
                    // Obtém o alvo do comando (outro cliente)
                    target_user = m[2].str();
                    bool found = false;

                    Socket *target;
                    // Tenta achar o cliente no canal
                    for (auto &mem_ptr : this->channels[my_channel].members) {
                        if (nick(mem_ptr.second) == target_user) {
                            found = true;
                            target = mem_ptr.first;
                            break;
                        }
                    }
                    if (!found) {
                        if (cmd == "invite") {
                            // Apenas continua se o cliente existir no servidor
                            if (this->users.find(target_user) != this->users.end()) {

                                if (this->channels[my_channel].isInviteOnly) {
                                    this->channels[my_channel].invited_users.insert(target_user);
                                }
                                this->send_chunk(my_nick + " convidou você a se juntar ao canal " + my_channel,
                                                 this->users[target_user]);
                                this->send_chunk(target_user + " foi convidado a se juntar a este canal!", client);
                            } else {
                                this->send_chunk(target_user + " não existe...", client);
                            }
                        } else {
                            this->send_chunk("O usuário solicitado não está neste canal!", client);
                        }
                    } else {
                        hash_value &target_tup = this->channels[my_channel].members[target];
                        // Verifica se o alvo é o próprio usuário
                        if (client == target) {
                            this->send_chunk("Você não pode ser alvo do comando de adm.", client);
                        } else {
                            if (cmd == "kick") {
                                this->change_channel(target, "#general");
                                this->send_chunk(my_nick + " expulsou você do canal.", target);
                                this->channel_notification(my_channel,
                                                           nick(target_tup) + " foi expulso do canal.");
                            } else if (cmd == "mute") {
                                muted(target_tup) = true;
                                this->channel_notification(my_channel, nick(target_tup) + " foi silenciado.");
                            } else if (cmd == "unmute") {
                                muted(target_tup) = false;
                                this->channel_notification(my_channel, nick(target_tup) + " não está mais silenciado.");
                            } else if (cmd == "whois") {
                                string target_ip = target->get_IP_address();
                                this->send_chunk("Usuário " + nick(target_tup) + " tem o endereco IP " + target_ip + ".",
                                                 client);
                            } else if (cmd == "invite") {
                                this->send_chunk(target_user + " ja está no canal!", client);
                            }
                        }
                    }
                }
            } else if (cmd == "invite") {
                // Você não pode utilizar o comando 'invite', caso não seja adm
                this->send_chunk("Você não pode utilizar o comando 'invite', caso não seja adm do canal", client);
            }
        } else {
            // Mensagem comum
            cout << my_channel + "-" + my_nick + ": " + buffer << endl;
            msg_pack.content = this->prepare_msg(buffer, client);
            msg_pack.sender = client;
            this->send_to_queue(msg_pack);
            // Apaga o buffer
            buffer.clear();
        }
        // Obtém o canal atual e as tuplas dos clientes
        my_channel = this->which_channel[client];
        myself = &(this->channels[my_channel].members[client]);
    }

    cout << "Conexao fechada com " << my_nick << endl;
    this->channel_notification(my_channel, "Usuário " + my_nick + " foi desconectado do servidor...");
}

/*
    Método para aceitar novos clientes (socket connections)
*/
void Server::accept() {
    Socket *client;
    thread new_thread;

    cout << "Aceitando novas conexões...\n";

    while (true) {
        // Aguarda ate receber novas conexoes. Então, cria um novo socket para a comunicação com esse cliente
        client = this->listener.accept_connection();
        // Abre uma thread para lidar com as mensagem desse cliente
        new_thread = this->receive_thread(client);
        new_thread.detach();
    }
}

/*
    Método para brodcasting de mensagens vindas de 'msg_queue'
*/
void Server::broadcast() {
    msg_info next_msg_pack;
    bool success = false;
    string c_name; // Nome do canal

    cout << "Broadcasting mensagens...\n";

    while (true) {
        // Previne conflitos
        this->mtx.lock();

        // Apenas executa quando há algo na fila
        if (!this->msg_queue.empty()) {
            // Obtém a próxima mensagem
            next_msg_pack = this->msg_queue.front();
            this->msg_queue.pop();

            // Apenas envia se o conteúdo não for vazio
            if ((int)next_msg_pack.content.size() > 0) {
                if (next_msg_pack.sender == nullptr) {
                    // Essa mensagem sera broadcasted no canal de 'msg_pack'
                    c_name = next_msg_pack.channel_name;
                    // Envia a mensagem para todos os clientes do canal...
                    for (auto it = this->channels[c_name].members.begin(); it != this->channels[c_name].members.end();
                         it++) {
                        hash_value &client = this->channels[c_name].members[it->first];
                        // Se eles tiverem a permissão.
                        if (allowed(client)) {
                            success = this->send_chunk(next_msg_pack.content, it->first);
                            // Se não conseguirmos enviar a mensagem ao cliente, ele quitou o servidor...
                            if (!success) {
                                alive(client) = false;
                            }
                        }
                    }
                } else {
                    // Obtém o nome do canal
                    string c_name = this->which_channel[next_msg_pack.sender];
                    // Obtém o status do remetente
                    hash_value &sender = this->channels[c_name].members[next_msg_pack.sender];
                    // Se o remetente é adm do canal
                    if (next_msg_pack.sender == this->channels[c_name].admin) {
                        // Envia para todos do canal...
                        for (auto it = this->channels[c_name].members.begin();
                             it != this->channels[c_name].members.end(); it++) {
                            hash_value &client = this->channels[c_name].members[it->first];
                            // Se eles tiverem a permissao.
                            if (allowed(client)) {
                                success = this->send_chunk("@" + next_msg_pack.content, it->first);
                                // Se não conseguirmos enviar a mensagem ao cliente, ele quitou o servidor...
                                if (!success) {
                                    alive(client) = false;
                                }
                            }
                        }
                    }
                    // Se o remetente não é adm e não está mudo...
                    else if (!muted(sender)) {
                        // Envia para todos do canal...
                        for (auto it = this->channels[c_name].members.begin();
                             it != this->channels[c_name].members.end(); it++) {
                            hash_value &client = this->channels[c_name].members[it->first];
                            // Se eles tiverem a permissao.
                            if (allowed(client)) {
                                success = this->send_chunk(next_msg_pack.content, it->first);
                                // Se não conseguirmos enviar a mensagem ao cliente, ele quitou o servidor...
                                if (!success) {
                                    alive(client) = false;
                                }
                            }
                        }
                    }
                }
            }
        }

        this->mtx.unlock();
    }
}

/* ----------------------------  Função de Execução ----------------------------- */

int main() {
    Server IRC(PORT);
    thread accept_t = IRC.accept_thread();
    thread broadcast_t = IRC.broadcast_thread();

    accept_t.join();
    broadcast_t.join();

    return 0;
}
