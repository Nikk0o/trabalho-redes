#include <iostream>
#include <thread>

#include <cstdio>
#include <exception>
#include <fcntl.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.hpp"


using std::size_t;
using std::string;
using std::cout;
using std::runtime_error;
using std::invalid_argument;


server::server(const char* name, int port) {
	this->name = string(name);
	this->port = port;

	// Cria o socket para os clientes se conectarem ao
	// gerenciador.
	// Socket ipv4 TCP
	//
	// Coloca também o socket em modo non-blocking, para
	// que as chamadas a recv() e accept() não bloqueiem
	// threads, assim evitando vazamento de memória quando
	// terminar o programa.
	sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sockfd < 0) {
		if (errno == EACCES || errno == EMFILE
			|| errno == ENFILE || errno == ENOMEM
			|| errno == ENOBUFS)

			throw runtime_error("Erro ao criar socket");
		else
			throw invalid_argument("Erro ao criar socket");
	}

	// Cria um semáforo para que chamadas de função
	// de abrir e fechar sockets possam ser execu-
	// tadas em threads diferentes
	sem_t* sem = sem_open("redes_server_sem", O_RDWR | O_CREAT, 0);
	if (sem == SEM_FAILED) {
		sem_unlink("redes_server_sem");
		sem = sem_open("redes_server_sem", O_RDWR | O_CREAT, 0);
	}

	if (sem == SEM_FAILED) {
		close(sockfd);
		throw runtime_error("Erro ao criar semáforo");
	}

	semaphore = sem;
	sem_post(semaphore);
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
		sem_destroy(semaphore);
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

	int new_socket;
	while(1) {
		if (!is_up())
			return -1;

		int new_socket = accept(sockfd, reinterpret_cast<sockaddr*>(&addr), &addr_size);
		if (new_socket < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;

			sem_unlink("redes_server_sem");
			if (errno == EINVAL)
				throw invalid_argument("Erro ao criar socket de cliente");
			else
				throw runtime_error("Erro ao criar socket de cliente");
		}
		break;
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
			close(fd);

			return;
		}
	}
}


void server::send(int fd, const void* buf, size_t size) {
	::send(fd, buf, size, 0);
}


int server::receive(int fd, void* buf, size_t max) {
	int result;

	while(1) {
		if (!is_up())
			return -2;

		result = recv(fd, buf, max, 0);
		if (result < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			if (errno == EINVAL)
				throw invalid_argument("Erro ao receber dados");
			else
				throw runtime_error("Erro ao receber dados");
		}
		break;
	}

	return result;
}
