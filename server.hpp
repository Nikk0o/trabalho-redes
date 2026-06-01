#ifndef SERVER_HPP
#define SERVER_HPP

#include "protocol.hpp"

#include <semaphore.h>
#include <vector>

	// Classe que controla o socket principal
	// no qual os clientes se conectam.
class server {

	// Nome para debug e saída
	std::string name;
	int port;
	int sockfd;

	sem_t* semaphore;
	bool up;
	std::vector<int> clients;

	public:
		server(const char*, int);
		~server();

		constexpr bool is_up() { return up; }

		void start();
		void stop();

		int wait_connection();
		void close_connection(int);

		void send(int, const void*, std::size_t);
		std::size_t receive(int, void*, std::size_t);
};

#define BACKLOG 10

#endif
