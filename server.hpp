#ifndef SERVER_HPP
#define SERVER_HPP

#include "protocol.hpp"

	// Classe que controla o socket principal
	// no qual os clientes se conectam.
class server {

	// Nome para debug e saída
	std::string name;
	int port;
	int sockfd;

	public:
		server(const char*, int);

		void start();
		void stop();

		int wait_connection();
		void close_connection(int);
};

#define BACKLOG 10

#endif
