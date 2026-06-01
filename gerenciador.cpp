#include "server.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <vector>


using namespace std;


class gerenciador {
	unique_ptr<server> srv;
	vector<pair<int, string>> lista_presenca;

	public:
		gerenciador(int);

		unique_ptr<packet> get_packet(int);
		void send_packet(int, packet);

		// Começa a aguardar conexões e
		// cria threads para cada cliente
		void listen();
};


int main(int argc, char* argv[]) {
	int port = stoi(argv[1]);
	gerenciador ger(port);
	ger.listen();

	return 0;
}


	// Inicializa o gerenciador e o servidor
gerenciador::gerenciador(int port) {
	srv = unique_ptr<server>(new server("Gerenciador", port));
	srv->start();
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
	{ throw 1; /* Mensagem não existe */ }
}


void gerenciador::send_packet(int fd, packet p) {
	string p_str = p.to_string();
	const char* c_str = p_str.c_str();
	srv->send(fd, c_str, p_str.size());
}


void listen_func(gerenciador& ger, server* srv) {
	while(1) {
		// Se o servidor, parar, retorne.
		if (!srv->is_up())
			return;

		int connct_sock = srv->wait_connection();
		if (connct_sock < 0) {
			srv->stop();
			abort(); // ?
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
		ger.send_packet(connct_sock, response);

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
	//thread thread_listen([this](){listen_func(*this, this->srv.get());});
	//thread_listen.detach();

	// srv->stop();
}
