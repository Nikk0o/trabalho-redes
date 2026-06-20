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
#include <sstream>
#include <unistd.h>
#include <sys/socket.h> // Garante o escopo das funções nativas de socket se necessário

using namespace std;

// Sockets globais para controle dos atuadores entre múltiplas threads
int socket_iluminacao = -1;
int socket_projetor = -1;
int socket_ar = -1;

class gerenciador {
public:
	unique_ptr<server> srv;
	vector<pair<int, string>> lista_presenca;
	sem_t* sem;

	gerenciador(int);
	~gerenciador();

	unique_ptr<packet> get_packet(int);
	void send_packet(int, packet&);
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
		srv = unique_ptr<server>(new server("Gerenciador", port));
		srv->start();
	}
	catch (exception& e) {
		perror(e.what());
		exit(1);
	}
}

gerenciador::~gerenciador() {
	sem_close(sem);
	sem_unlink("ger_semaforo");
}

// Função responsável por ler dados textuais brutos do socket e transformá-los em Structs de mensagens
unique_ptr<packet> gerenciador::get_packet(int fd) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	
	// Recebe os dados brutos da rede
	int bytes_received = srv->receive(fd, buffer, sizeof(buffer) - 1);
	if (bytes_received <= 0) {
		return unique_ptr<packet>(nullptr); // Conexão fechada ou erro
	}

	string msg(buffer);
	stringstream ss(msg);
	string linha;

	string protocolo = "", tipo_msg = "";
	
	// Parsing do cabeçalho de 2 linhas obrigatórias
	if (getline(ss, linha) && linha.find("PROTOCOLO: ") == 0) {
		protocolo = extrair_valor(linha, "PROTOCOLO: ");
	}
	if (getline(ss, linha) && linha.find("TIPO_MSG: ") == 0) {
		tipo_msg = extrair_valor(linha, "TIPO_MSG: ");
	}

	if (protocolo != "SMARTCLASS/1.0") {
		return unique_ptr<packet>(nullptr);
	}

	// Identificação de Mensagem por Mensagem baseada no Tipo
	if (tipo_msg == "REQ_CON") {
		unique_ptr<req_con> p(new req_con());
		while (getline(ss, linha)) {
			if (linha.find("ORIGEM_ID: ") == 0) strcpy(p->ip, extrair_valor(linha, "ORIGEM_ID: ").c_str());
			if (linha.find("FUNCAO: ") == 0) strcpy(p->funcao, extrair_valor(linha, "FUNCAO: ").c_str());
		}
		return move(p);
	} 
	else if (tipo_msg == "SENS_PRESENCA") {
		unique_ptr<sens_presenca> p(new sens_presenca());
		while (getline(ss, linha)) {
			if (linha.find("DETECTADO: ") == 0) {
				p->detectado = (extrair_valor(linha, "DETECTADO: ") == "TRUE");
			}
		}
		return move(p);
	} 
	else if (tipo_msg == "SENS_CARTAO") {
		unique_ptr<sens_cartao> p(new sens_cartao());
		while (getline(ss, linha)) {
			if (linha.find("NUMERO: ") == 0) p->numero = stoi(extrair_valor(linha, "NUMERO: "));
			if (linha.find("NOME: ") == 0) p->nome = extrair_valor(linha, "NOME: ");
		}
		return move(p);
	} 
	else if (tipo_msg == "SENS_CHAVE") {
		unique_ptr<sens_chave> p(new sens_chave());
		while (getline(ss, linha)) {
			if (linha.find("ESTADO: ") == 0) {
				p->estado = (extrair_valor(linha, "ESTADO: ") == "ON");
			}
		}
		return move(p);
	} 
	else if (tipo_msg == "GET_PRESENCA") {
		return unique_ptr<packet>(new get_presenca());
	}

	return unique_ptr<packet>(nullptr);
}

void gerenciador::send_packet(int fd, packet& p) {
	string p_str = p.to_string();
	srv->send(fd, p_str.c_str(), p_str.size());
}

// Thread dedicada em Background para cada cliente conectado e autenticado
void thread_cliente_func(gerenciador* ger, int client_sock, string funcao) {
	cout << "[THREAD] Monitorando dispositivo: " << funcao << " na socket " << client_sock << endl;

	while (true) {
		unique_ptr<packet> p = ger->get_packet(client_sock);
		if (!p) {
			cout << "[THREAD] Dispositivo " << funcao << " desconectou da socket " << client_sock << endl;
			// Limpa referências se for um atuador
			if (funcao == "ILUMINACAO") socket_iluminacao = -1;
			if (funcao == "PROJETOR") socket_projetor = -1;
			if (funcao == "AR_CONDICIONADO") socket_ar = -1;
			break;
		}

		string tipo = p->type_to_string();
		cout << "[LOG] Pacote recebido de " << funcao << ": " << tipo << endl;

		// CORREÇÃO: Usando dynamic_cast nativo com ponteiros brutos para extrair das unique_ptrs com segurança
		if (tipo == "SENS_PRESENCA") {
			sens_presenca* p_presenca = dynamic_cast<sens_presenca*>(p.get());
			if (p_presenca && p_presenca->detectado) {
				cout << "[AUTOMAÇÃO] Presença Detectada! Ligando aparelhos..." << endl;
				
				if (socket_iluminacao != -1) {
					comando cmd; cmd.alvo = ILUMINACAO; cmd.acao = true;
					ger->send_packet(socket_iluminacao, cmd);
				}
				if (socket_ar != -1) {
					comando cmd; cmd.alvo = AR_CONDICIONADO; cmd.acao = true;
					ger->send_packet(socket_ar, cmd);
				}
			}
		}
		else if (tipo == "SENS_CARTAO") {
			sens_cartao* p_cartao = dynamic_cast<sens_cartao*>(p.get());
			if (p_cartao) {
				sem_wait(ger->sem); 
				ger->lista_presenca.push_back({p_cartao->numero, p_cartao->nome});
				sem_post(ger->sem);
				cout << "[AUTOMAÇÃO] Aluno " << p_cartao->nome << " adicionado à lista de presença." << endl;
			}
		}
		else if (tipo == "SENS_CHAVE") {
			sens_chave* p_chave = dynamic_cast<sens_chave*>(p.get());
			if (p_chave) {
				if (p_chave->estado) { 
					cout << "[AUTOMAÇÃO] Chave ligada: Ativando Projetor e Apagando Luzes." << endl;
					if (socket_projetor != -1) { comando cmd; cmd.alvo = PROJETOR; cmd.acao = true; ger->send_packet(socket_projetor, cmd); }
					if (socket_iluminacao != -1) { comando cmd; cmd.alvo = ILUMINACAO; cmd.acao = false; ger->send_packet(socket_iluminacao, cmd); }
				} else { 
					cout << "[AUTOMAÇÃO] Chave desligada: Desativando Projetor e Acendendo Luzes." << endl;
					if (socket_projetor != -1) { comando cmd; cmd.alvo = PROJETOR; cmd.acao = false; ger->send_packet(socket_projetor, cmd); }
					if (socket_iluminacao != -1) { comando cmd; cmd.alvo = ILUMINACAO; cmd.acao = true; ger->send_packet(socket_iluminacao, cmd); }
				}
			}
		}
		else if (tipo == "GET_PRESENCA") {
			cout << "[PROFESSOR] Enviando lista de presença atual..." << endl;
			string resposta = "PROTOCOLO: SMARTCLASS/1.0\nTIPO_MSG: RESP_PRESENCA\n";
			
			sem_wait(ger->sem);
			resposta += "TOTAL_ALUNOS: " + to_string(ger->lista_presenca.size()) + "\n";
			for (size_t i = 0; i < ger->lista_presenca.size(); ++i) {
				resposta += "ALUNO_" + to_string(i + 1) + ": " + to_string(ger->lista_presenca[i].first) + " " + ger->lista_presenca[i].second + "\n";
			}
			sem_post(ger->sem);
			resposta += "\n"; 

			// CORREÇÃO: Utilizando a função srv->send interna do seu servidor para evitar problemas de escopo global
			ger->srv->send(client_sock, resposta.c_str(), resposta.size());
		}
	}

	ger->srv->close_connection(client_sock);
}

void listen_func(gerenciador& ger, server* srv) {
	while (1) {
		int connct_sock;
		try {
			connct_sock = srv->wait_connection();
			if (connct_sock < 0)
				continue;
		}
		catch (exception& e) {
			perror(e.what());
			return;
		}

		unique_ptr<packet> p = move(ger.get_packet(connct_sock));
		if (!p) {
			cout << "[AVISO] Pacote recebido inválido ou nulo durante o handshake." << endl;
			srv->close_connection(connct_sock);
			continue;
		}

		// CORREÇÃO: Cast usando ponteiro bruto para validar o Handshake inicial
		req_con* handsh = dynamic_cast<req_con*>(p.get());
		if (!handsh) {
			cout << "[AVISO] Primeiro pacote não é um REQ_CON válido." << endl;
			srv->close_connection(connct_sock);
			continue;
		}

		string funcao_disp = string(handsh->funcao);
		if (funcao_disp == "ILUMINACAO") socket_iluminacao = connct_sock;
		else if (funcao_disp == "PROJETOR") socket_projetor = connct_sock;
		else if (funcao_disp == "AR_CONDICIONADO") socket_ar = connct_sock;

		con_ack response;
		response.status = true;
		try {
			ger.send_packet(connct_sock, response);
			cout << "[LOG] Handshake bem-sucedido com dispositivo: " << funcao_disp << " na socket " << connct_sock << endl;

			thread t_cliente(thread_cliente_func, &ger, connct_sock, funcao_disp);
			t_cliente.detach();
		}
		catch (exception& e) {
			perror(e.what());
			srv->close_connection(connct_sock);
			continue;
		}
	}
}

void gerenciador::listen() {
	thread main_listen(listen_func, ref(*this), srv.get());
	main_listen.join();
}