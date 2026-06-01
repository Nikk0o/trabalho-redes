#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>

enum alvos {
	ILUMINACAO,
	PROJETOR,
	AR_CONDICIONADO
};

// enum packet_type {};

	// struct que representa
	// de um pacote. Precisa
	// ser convertida antes de
	// ser enviada.
struct packet {
	std::string to_string_header() {
		return std::string("SMARTCLASS/1.0\n") +
			   "TIPO_MSG: " +
			   type_to_string()
			   + "\n";
	};

	virtual std::string type_to_string() { return ""; }
	virtual std::string to_string() { return ""; }
};

struct req_con : packet {
	char ip[16];
	char funcao[16];
};

struct con_ack : packet {
	bool status;
};

struct sens_presenca : packet {
	bool detectado;
};

struct sens_cartao : packet {
	int numero;
	std::string nome;
};

struct sens_chave : packet {
	bool estado;
};

struct comando : packet {
	alvos alvo;
	bool acao;
};

struct get_presenca : packet {

};

struct resp_presenca : packet {
	int total_alunos;
	std::vector<std::pair<int, std::string>> alunos;
};

#endif
