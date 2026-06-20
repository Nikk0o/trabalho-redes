#include <iostream>
#include <thread>

#include <cstdio>
#include <exception>
#include <fcntl.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.hpp"

using std::size_t;
using std::string;
using std::cout;
using std::runtime_error;
using std::invalid_argument;

server::server(const char* name, int port) {
	this->name = string(name);
	this->port = port;

	// Cria o socket para os clientes se conectarem ao gerenciador.
	// Socket IPv4 TCP estruturado em modo BLOQUEANTE (blocking).
	//
	// Mudei para o modo bloqueante para simplificar a lógica de leitura e escrita, 
	// já que cada cliente terá uma thread dedicada.
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		if (errno == EACCES || errno == EMFILE
			|| errno == ENFILE || errno == ENOMEM
			|| errno == ENOBUFS)
			throw runtime_error("Erro ao criar socket");
		else
			throw invalid_argument("Erro ao criar socket");
	}

	// Configura o socket para permitir o reuso imediato do endereço e da porta,
	// evitando o erro de "Address already in use" ao reiniciar o servidor seguidamente.
	int opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("Erro ao configurar SO_REUSEADDR");
	}

	// Cria um semáforo para que chamadas de função de abrir e fechar 
	// sockets de clientes possam ser executadas de forma segura e síncrona
	// a partir de threads diferentes (evitando concorrência no vetor 'clients').
	sem_t* sem = sem_open("redes_server_sem", O_CREAT, 0666, 1);
	if (sem == SEM_FAILED) {
		sem_unlink("redes_server_sem");
		sem = sem_open("redes_server_sem", O_CREAT, 0666, 1);
	}

	if (sem == SEM_FAILED) {
		close(sockfd);
		throw runtime_error("Erro ao criar semáforo");
	}

	semaphore = sem;
	sem_post(semaphore);
	up = false;
}

// Inicia o servidor vinculando-o à porta especificada e escutando em qualquer interface (INADDR_ANY)
void server::start() {
	cout << "Iniciando " << name << "." << std::endl;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port); // Conversão da porta para o formato big-endian da rede

	if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
		sem_unlink("redes_server_sem");
		stop();

		if (errno == EADDRINUSE || errno == EINVAL || errno == EFAULT)
			throw invalid_argument("Erro ao fazer bind()");
		else
			throw runtime_error("Erro ao fazer bind()");
	}

	up = true;
	cout << name << " iniciado na porta " << port << "." << std::endl;
}

void server::stop() {
	if (!up)
		return;

	cout << "Parando " << name << "." << std::endl;
	up = false;
	close(sockfd);

	// Fecha ativamente a socket de todos os clientes que ainda estão conectados
	while (clients.size())
		close_connection(clients[0]);

	sem_close(semaphore);
	cout << name << " Finalizado." << std::endl;
}

server::~server() {
	stop();
}

int server::wait_connection() {
	cout << "Esperando conexão..." << std::endl;

	if (listen(sockfd, BACKLOG) < 0) {
		throw runtime_error("Erro ao escutar (listen)");
	}

	sockaddr_in addr;
	socklen_t addr_size = sizeof(addr);

	// No modo bloqueante, o accept() suspende a execução da thread principal aqui 
	// de forma eficiente até que um novo dispositivo cliente solicite uma conexão TCP.
	int new_socket = accept(sockfd, reinterpret_cast<sockaddr*>(&addr), &addr_size);
	if (new_socket < 0) {
		if (errno == EINVAL)
			throw invalid_argument("Erro ao criar socket de cliente");
		else
			throw runtime_error("Erro ao criar socket de cliente");
	}

	// Adiciona de forma segura a nova conexão à lista de clientes conectados.
	sem_wait(semaphore);
	clients.push_back(new_socket);
	sem_post(semaphore);

	cout << "Conexão estabelecida na socket " << new_socket << "." << std::endl;
	
	// Retorna o file descriptor do socket atribuído à conexão estabelecida.
	return new_socket;
}

// Fecha a conexão com um cliente específico, removendo-o da lista monitorada.
void server::close_connection(int fd) {
	if (fd == sockfd)
		return;

	for (int i = 0; i < clients.size(); i++) {
		if (clients[i] == fd) {
			sem_wait(semaphore);
			clients.erase(clients.begin() + i);
			sem_post(semaphore);
			close(fd);
			return;
		}
	}
}

void server::send(int fd, const void* buf, size_t size) {
	::send(fd, buf, size, 0);
}

int server::receive(int fd, void* buf, size_t max) {
	// Chamada bloqueante para o recv. A thread dedicada a este cliente ficará dormindo 
	// até que o dispositivo envie dados textuais do protocolo SmartClass.
	// Se o cliente desconectar de forma limpa, o retorno será 0.
	int result = recv(fd, buf, max, 0);
	if (result < 0) {
		if (errno == EINVAL)
			throw invalid_argument("Erro ao receber dados");
		else
			throw runtime_error("Erro ao receber dados");
	}
	return result;
}