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
		sem = sem_open("ger_semaforo", O_CREAT, 0666, 1);
		if (sem == SEM_FAILED) {
			sem_unlink("ger_semaforo");
			sem = sem_open("ger_semaforo", O_CREAT, 0666, 1);
		}

		if (sem == SEM_FAILED)
			throw runtime_error("Erro ao criar o semáforo do gerenciador");

		srv = unique_ptr<server>(new server("Gerenciador", port));
	}
	catch (exception e) {
		perror(e.what());
		if (srv) srv->stop();
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

	int bytes = srv->receive(fd, static_cast<void*>(&c), 1);
	if (bytes <= 0) return "";

	while(c != stop) {
		result += c;
		bytes = srv->receive(fd, static_cast<void*>(&c), 1);
		if (bytes <= 0) break;
	}

	result.shrink_to_fit();
	return result;
}

// Lê um pacote recebido
unique_ptr<packet> gerenciador::get_packet(int fd) {
	string protocolo = "SMARTCLASS/1.0";
	string tipo_msg = "TIPO_MSG:";

	// Lê o cabeçalho
	string protocolo_rec = get_until(fd, '\n', srv.get());
	if (protocolo_rec.find(protocolo) == string::npos) {
		return unique_ptr<packet>(nullptr);
	}

	// Lê tipo_msg
	string tipo_msg_rec = get_until(fd, ' ', srv.get());
	string tipo = get_until(fd, '\n', srv.get());

	if (tipo.find("REQ_CON") != string::npos) {
		// Processa pacote de requisição de conexão
		unique_ptr<req_con> req(new req_con());
		
		// Lê "IP: "
		get_until(fd, ' ', srv.get());
		string ip_val = get_until(fd, '\n', srv.get());
		
		// Lê "FUNCAO: "
		get_until(fd, ' ', srv.get());
		string funcao_val = get_until(fd, '\n', srv.get());

		strncpy(req->ip, ip_val.c_str(), sizeof(req->ip) - 1);
		strncpy(req->funcao, funcao_val.c_str(), sizeof(req->funcao) - 1);
		return move(req);
	}
	else if (tipo.find("SENS_PRESENCA") != string::npos) {
		// Processa mensagem do sensor de presença
		unique_ptr<sens_presenca> presenca(new sens_presenca());
		get_until(fd, ' ', srv.get()); // "DETECTADO: "
		string det = get_until(fd, '\n', srv.get());
		presenca->detectado = (det.find("SIM") != string::npos);
		return move(presenca);
	}
	else if (tipo.find("SENS_CARTAO") != string::npos) {
		// Processa mensagem do sensor de cartão
		unique_ptr<sens_cartao> sc(new sens_cartao());
		get_until(fd, ' ', srv.get()); // "NUMERO: "
		sc->numero = stoi(get_until(fd, '\n', srv.get()));
		get_until(fd, ' ', srv.get()); // "NOME: "
		sc->nome = get_until(fd, '\n', srv.get());
		return move(sc);
	}
	else if (tipo.find("GET_PRESENCA") != string::npos) {
		// Processa mensagem de requisição de
		// lista de presença
		return unique_ptr<packet>(new get_presenca());
	}
	
	return unique_ptr<packet>(nullptr);
}

void gerenciador::send_packet(int fd, packet p) {
	string p_str = p.to_string();
	srv->send(fd, p_str.c_str(), p_str.size());
}

void listen_func(gerenciador& ger, server* srv) {
	while(1) {
		int connct_sock;
		try {
			connct_sock = srv->wait_connection();
			if (connct_sock < 0) continue;
		}
		catch (exception& e) {
			perror(e.what());
			return;
		}

		// O primeiro pacote que o gerenciador espera receber de um 
		// dispositivo é o REQ_CON, que contém o tipo do dispositivo e seu IP.
		shared_ptr<packet> p = move(ger.get_packet(connct_sock));
		
		// Se o pacote for inválido ou nulo, fecha a conexão e continua esperando por novas conexões.
		if (!p) {
			cout << "[AVISO] Pacote recebido inválido ou nulo." << endl;
			srv->close_connection(connct_sock);
			continue;
		}

		// Tenta fazer o cast do pacote recebido para um REQ_CON, 
		// que é o esperado para o handshake.
		shared_ptr<req_con> handsh = dynamic_pointer_cast<req_con>(p);
		
		// Se o primeiro pacote não for um REQ_CON válido, 
		// fecha a conexão e continua esperando por novas conexões.
		if (!handsh) {
			cout << "[AVISO] Primeiro pacote não é um REQ_CON válido." << endl;
			srv->close_connection(connct_sock);
			continue;
		}

		con_ack response;
		response.status = true;
		try {
			ger.send_packet(connct_sock, response);
			cout << "[LOG] Handshake bem-sucedido com dispositivo: " << handsh->funcao << endl;
		}
		catch (exception& e) {
			perror(e.what());
			srv->close_connection(connct_sock);
			continue;
		}

		string funcao = string(handsh->funcao);
		// Criar as threads para monitorar o dispositivo baseado no tipo.
	}
}

void gerenciador::listen() {
	srv->start();
	thread thread_listen([this](){ listen_func(*this, this->srv.get()); });
	thread_listen.join();
	cout << "Gerenciador parou de ouvir." << std::endl;
}