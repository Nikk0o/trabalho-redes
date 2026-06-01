#include "server.hpp"

#include <iostream>
#include <thread>

using namespace std;

	// Função que espera uma conexão no socket
void listen_func(server& srv) {
	while(1) {
		int socket = srv.wait_connection();

		// Cria uma thread para a conexão nova
	}
}

int main(int argc, char* argv[]) {
	int port = stoi(argv[1]);
	server ger_socket("Gerenciador", port);
	ger_socket.start();

	// Cria thread para esperar conexões
	thread listen_thread(listen_func);
	listen_thread.detach();

	return 0;
}
