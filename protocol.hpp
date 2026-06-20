#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <sstream>

enum alvos {
	ILUMINACAO,
	PROJETOR,
	AR_CONDICIONADO
};

// Função auxiliar global para extrair o valor após o rótulo (ex: "FUNCAO: SENSOR" -> "SENSOR")
inline std::string extrair_valor(const std::string& linha, const std::string& rotulo) {
	if (linha.find(rotulo) == 0) {
		return linha.substr(rotulo.length());
	}
	return "";
}

struct packet {
	std::string to_string_header() {
		return std::string("PROTOCOLO: SMARTCLASS/1.0\n") +
			   "TIPO_MSG: " +
			   type_to_string()
			   + "\n";
	};

	virtual std::string type_to_string() { return ""; }
	virtual std::string to_string() { return ""; }
};

struct req_con : packet {
	char ip[16];
	char funcao[24]; // Aumentado para suportar AR_CONDICIONADO com folga

	std::string type_to_string() override { return "REQ_CON"; }
	std::string to_string() override {
		return to_string_header() +
			   "ORIGEM_ID: " + std::string(ip) + "\n" +
			   "FUNCAO: " + std::string(funcao) + "\n";
	}
};

struct con_ack : packet {
	bool status;

	std::string type_to_string() override { return "CON_ACK"; }
	std::string to_string() override {
		return to_string_header() +
			   "STATUS: " + std::string(status ? "SUCESS" : "FAILED") + "\n";
	}
};

struct sens_presenca : packet {
	bool detectado;

	std::string type_to_string() override { return "SENS_PRESENCA"; }
	std::string to_string() override {
		return to_string_header() +
			   "DETECTADO: " + std::string(detectado ? "TRUE" : "FALSE") + "\n";
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
			   "ESTADO: " + std::string(estado ? "ON" : "OFF") + "\n";
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
			   "ACAO: " + std::string(acao ? "ON" : "OFF") + "\n";
	}
};

struct get_presenca : packet {
	std::string type_to_string() override { return "GET_PRESENCA"; }
	std::string to_string() override {
		return to_string_header();
	}
};

#endif