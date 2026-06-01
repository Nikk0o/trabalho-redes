#include <iostream>
#include <thread>

#include <cstdio>
#include <fcntl.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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
	if (sockfd < 0) {
		perror("Erro ao abrir socket");
		abort(); // ?
	}

	// Cria um semáforo para que chamadas de função
	// de abrir e fechar sockets possam ser execu-
	// tadas em threads diferentes
	sem_t* sem = sem_open("redes_server_sem", O_RDWR | O_CREAT, 0);
	if (sem == SEM_FAILED)
		sem_unlink("redes_server_sem");

	sem = sem_open("redes_server_sem", O_RDWR | O_CREAT, 0);
	if (sem == SEM_FAILED) {
		perror("Erro ao criar semáforo");
		abort(); // ?
	}

	semaphore = sem;
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
		perror("Erro ao fazer bind() no socket do servidor");
		sem_destroy(semaphore);
		abort(); // ?
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

	// faz isso e força as threads a terminarem talvez
	// ou só dá join
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

	listen(sockfd, BACKLOG);

	// não sei pra que isso serviria, já que a gente manda com os
	// file descriptors, não com o ip.
	// esses campos de addr podem ser nullptr.
	sockaddr_in addr;
	socklen_t addr_size;
	int new_socket = accept(sockfd, reinterpret_cast<sockaddr*>(&addr), &addr_size);
	if (new_socket < 0) {
		perror("Erro ao criar socket de cliente");
		sem_unlink("redes_server_sem");
		abort(); // ?
	}

	// Adiciona a conexão à lista de clientes conectados.
	sem_wait(semaphore);
	clients.push_back(new_socket);
	sem_post(semaphore);

	cout << "Conexão estabelecida na socket " << new_socket << "." << std::endl;

	// Retorna o socket atribuído à conexão estabelecida.
	return new_socket;
}


	// Fecha a conexão com um cliente específico.
	// Não sei em que caso seria útil no projeto.
void server::close_connection(int fd) {
	if (fd == sockfd)
		return;

	for (int i = 0; i < clients.size(); i++) {
		if (clients[i] == fd) {
			// Remove a conexão da lista.
			sem_wait(semaphore);
			clients.erase(clients.begin()+i);
			sem_post(semaphore);

			return;
		}
	}
}


void server::send(int fd, const void* buf, size_t size) {
	::send(fd, buf, size, 0);
}


size_t server::receive(int fd, void* buf, size_t max) {
	return recv(fd, buf, max, 0);
}
