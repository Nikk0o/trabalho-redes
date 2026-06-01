#include <iostream>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>

#include "server.hpp"


using std::size_t;
using std::string;
using std::cout;


server::server(const char* name, int port) {
	this->name = string(name);
	this->port = port;

	// Cria o socket para os clientes se conectarem ao
	// gerenciador.
	// Socket ipv4 TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		abort(); // ?
}


	// Inicia o servidor usando ipv4 e escutando no
	// endereço de loopback.
void server::start() {

	cout << "Iniciando " << name << "." << std::endl;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = port;
	if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
		< 0) {
		abort(); // ?
	}

	cout << name << " iniciado na porta " << port << "." << std::endl;
}


void server::stop() {}


int server::wait_connection() {
	cout << "Esperando conexão..." << std::endl;

	listen(sockfd, BACKLOG);

	// não sei pra que isso serviria, já que a gente manda com os
	// file descriptors, não com o ip.
	// esses campos de addr podem ser nullptr.
	sockaddr_in addr;
	socklen_t addr_size;
	int new_socket = accept(sockfd, reinterpret_cast<sockaddr*>(&addr), &addr_size);
	if (new_socket < 0)
		abort(); // ?

	cout << "Conexão estabelecida na socket " << new_socket << "." << std::endl;

	// Retorna o socket atribuído à conexão estabelecida.
	return new_socket;
}


	// Fecha a conexão com um cliente específico.
	// Não sei em que caso seria útil no projeto.
void server::close_connection(int fd) {}


	// Envia um pacote
void send_packet(int fd, packet& p) {
	string p_str = p.to_string();
	const char* c_str = p_str.c_str();
	send(fd, c_str, p_str.size(), 0);

	delete[] c_str;
}


	// Lê o socket até que um caractere stop seja
	// encontrado.
string get_until(int fd, char stop) {
	char c;
	string result;
	result.reserve(20);

	recv(fd, static_cast<void*>(&c), 1, 0);
	while(c != stop) {
		result += c;
		recv(fd, static_cast<void*>(&c), 1, 0);
	}

	result.shrink_to_fit();
	return result;
}


	// Lê um pacote recebido
packet get_packet(int fd) {
	string protocolo = "SMARTCLASS/1.0";
	string tipo_msg = "TIPO_MSG: ";

	// Lê o cabeçalho
	string protocolo_rec = get_until(fd, '\n');
	if (protocolo_rec.compare(protocolo))
	{ /* Cabeçalho errado */ }

	// Lê tipo_msg
	string tipo_msg_rec = get_until(fd, ' ');
	if (tipo_msg_rec.compare(tipo_msg))
	{ /* Cabeçalho errado */ }

	string tipo = get_until(fd, '\n');
	if (!string("REQ_CON").compare(tipo)) {
		// Processa pacote de requisição de conexão
	}
	else if (!string("SENS_PRESENCA").compare(tipo)) {
		// Processa mensagem do sensor de presença
	}
	else if (!string("SENS_CARTAO").compare(tipo)) {
		// Processa mensagem do sensor de cartão
	}
	else if (!string("SENS_CHAVE").compare(tipo)) {
		// Processa mensgem do sensor de chave
	}
	else if (!string("GET_PRESENCA").compare(tipo)) {
		// Processa mensagem de requisição de
		// lista de presença
	}
	else
	{ /* Mensagem não existe */ }
}
