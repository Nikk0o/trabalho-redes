#include "server.hpp"

#include <cstring>
#include <iostream>
#include <list>
#include <malloc.h>
#include <memory>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <semaphore.h>


using namespace std;


class gerenciador {
	unique_ptr<server> srv;
	vector<pair<int, string>> lista_presenca;

	// semáforo que vai ser usado para acessar
	// os dados compartilhados do gerenciador
	// pelas threads.
	sem_t* sem;

	public:
		gerenciador(int);
		~gerenciador();

		unique_ptr<packet> get_packet(int);
		void send_packet(int, packet);

		// Começa a aguardar conexões e
		// cria threads para cada cliente
		void listen();
};


int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Coloque a porta desejada no primeiro parâmetro do programa." << std::endl;
		exit(1);
	}

	int port = stoi(argv[1]);
	gerenciador ger(port);
	ger.listen();

	return 0;
}


	// Inicializa o gerenciador e o servidor
gerenciador::gerenciador(int port) {
	try {
		sem = sem_open("ger_semaforo", O_CREAT);
		if (sem == SEM_FAILED) {
			sem_unlink("ger_semaforo");
			sem = sem_open("ger_semaforo", O_CREAT);
		}

		if (sem == SEM_FAILED)
			throw runtime_error("Erro ao criar o semáforo do gerenciador");

		srv = unique_ptr<server>(new server("Gerenciador", port));
		srv->start();
	}
	catch (exception e) {
		perror(e.what());
		srv->stop();
		exit(1);
	}

	sem_post(sem);
}


gerenciador::~gerenciador() {
	sem_close(sem);
}


	// Lê o socket até que um caractere stop seja
	// encontrado.
string get_until(int fd, char stop, server* srv) {
	char c;
	string result;
	result.reserve(20);

	srv->receive(fd, static_cast<void*>(&c), 1);
	while(c != stop) {
		result += c;
		srv->receive(fd, static_cast<void*>(&c), 1);
	}

	result.shrink_to_fit();
	return result;
}


	// Lê um pacote recebido
unique_ptr<packet> gerenciador::get_packet(int fd) {

	string protocolo = "SMARTCLASS/1.0";
	string tipo_msg = "TIPO_MSG: ";

	// Lê o cabeçalho
	string protocolo_rec = get_until(fd, '\n', srv.get());
	if (protocolo_rec.compare(protocolo))
	{ /* Cabeçalho errado */ }

	// Lê tipo_msg
	string tipo_msg_rec = get_until(fd, ' ', srv.get());
	if (tipo_msg_rec.compare(tipo_msg))
	{ /* Cabeçalho errado */ }

	string tipo = get_until(fd, '\n', srv.get());
	if (!string("REQ_CON").compare(tipo)) {
		// Processa pacote de requisição de conexão
		unique_ptr<req_con> req(new req_con());
		return req;
	}
	else if (!string("SENS_PRESENCA").compare(tipo)) {
		// Processa mensagem do sensor de presença
		unique_ptr<sens_presenca> presenca(new sens_presenca());
		return presenca;
	}
	else if (!string("SENS_CARTAO").compare(tipo)) {
		// Processa mensagem do sensor de cartão
		unique_ptr<sens_cartao>sc(new sens_cartao());
		return sc;
	}
	else if (!string("SENS_CHAVE").compare(tipo)) {
		// Processa mensgem do sensor de chave
		unique_ptr<sens_chave> sc(new sens_chave());
		return sc;
	}
	else if (!string("GET_PRESENCA").compare(tipo)) {
		// Processa mensagem de requisição de
		// lista de presença
		unique_ptr<get_presenca>gp(new get_presenca());
		return gp;
	}
	else
	{ return unique_ptr<packet>(nullptr); /* Mensagem não existe */ }
}


void gerenciador::send_packet(int fd, packet p) {
	string p_str = p.to_string();
	const char* c_str = p_str.c_str();
	srv->send(fd, c_str, p_str.size());
}


void listen_func(gerenciador& ger, server* srv) {
	int i = 0;
	while(1) {
		int connct_sock;

		try {
			connct_sock = srv->wait_connection();
			if (connct_sock < 0)
				continue;
		}
		catch (exception e) {
			perror(e.what());
			return;
		}

		shared_ptr<packet> p = move(ger.get_packet(connct_sock));
		shared_ptr<req_con> handsh = dynamic_pointer_cast<req_con>(p);
		if (!handsh) {
			// Não segue o protocolo
			//
			// talvez seja a mensagem pedindo a lista
			// de presença tbm, talvez verificar isso
		}

		con_ack response;
		response.status = true;
		try {
			ger.send_packet(connct_sock, response);
		}
		catch (exception e) {
			perror(e.what());
			return;
		}

		string funcao = string(handsh->funcao);
		if (!funcao.compare("")) {
			// Cria uma thread para se comunicar com
			// um sensor
		}
		else if (!funcao.compare("")) {
			// Cria uma thread para se comunicar com
			// um atuador
		}
	}
}


void gerenciador::listen() {
	thread thread_listen([this](){listen_func(*this, this->srv.get());});

	thread_listen.join();
	cout << "Gerenciador parou de ouvir." << std::endl;
}
