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

// conversão de string
// para struct e vice-versa
// para cada tipo de pacote

struct req_con : packet {
	char ip[16];
	char funcao[16];

	std::string type_to_string() override { return "REQ_CON"; }
	std::string to_string() override {
		return to_string_header() +
			   "IP: " + std::string(ip) + "\n" +
			   "FUNCAO: " + std::string(funcao) + "\n";
	}
};

struct con_ack : packet {
	bool status;

	std::string type_to_string() override { return "CON_ACK"; }
	std::string to_string() override {
		return to_string_header() +
			   "STATUS: " + std::string(status ? "SUCESSO" : "FALHA") + "\n";
	}
};

struct sens_presenca : packet {
	bool detectado;

	std::string type_to_string() override { return "SENS_PRESENCA"; }
	std::string to_string() override {
		return to_string_header() +
			   "DETECTADO: " + std::string(detectado ? "SIM" : "NAO") + "\n";
	}
};

struct sens_cartao : packet {
	int numero;
	std::string nome;

	std::string type_to_string() override { return "SENS_CARTAO"; }
	std::string to_string() override {
		return to_string_header() +
			   "NUMERO: " + std::to_string(numero) + "\n" +
			   "NOME: " + nome + "\n";
	}
};

struct sens_chave : packet {
	bool estado;

	std::string type_to_string() override { return "SENS_CHAVE"; }
	std::string to_string() override {
		return to_string_header() +
			   "ESTADO: " + std::string(estado ? "ABERTA" : "FECHADA") + "\n";
	}
};

struct comando : packet {
	alvos alvo;
	bool acao;

	std::string type_to_string() override { return "COMANDO"; }
	std::string to_string() override {
		std::string alvo_str;
		switch (alvo) {
			case ILUMINACAO: alvo_str = "ILUMINACAO"; break;
			case PROJETOR: alvo_str = "PROJETOR"; break;
			case AR_CONDICIONADO: alvo_str = "AR_CONDICIONADO"; break;
		}
		return to_string_header() +
			   "ALVO: " + alvo_str + "\n" +
			   "ACAO: " + std::string(acao ? "LIGAR" : "DESLIGAR") + "\n";
	}
};

struct get_presenca : packet {

	std::string type_to_string() override { return "GET_PRESENCA"; }
	std::string to_string() override {
		return to_string_header();
	}
};

struct resp_presenca : packet {
	int total_alunos;
	std::vector<std::pair<int, std::string>> alunos;

	std::string type_to_string() override { return "RESP_PRESENCA"; }
	std::string to_string() override {
		std::string result = to_string_header() +
							 "TOTAL_ALUNOS: " + std::to_string(total_alunos) + "\n";
		for (const auto& aluno : alunos) {
			result += "ALUNO: " + std::to_string(aluno.first) + " - " + aluno.second + "\n";
		}
		return result;
	}
};

#endif
