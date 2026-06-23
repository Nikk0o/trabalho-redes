#include <ctime>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>
#include <thread>
#include "protocol.hpp" 

using namespace std;

// Função auxiliar para enviar pacotes 
// Transforma o pacote em string e envia pelo socket,
// além de imprimir o conteúdo do pacote no console para fins de depuração.
void enviar_pacote(int fd, packet& p) {
    string p_str = p.to_string();
    send(fd, p_str.c_str(), p_str.size(), 0);
    
    // Variáveis estáticas retêm o valor entre as chamadas da função
    static string ultimo_conteudo_enviado = "";

    // Só imprime na tela se a string atual for diferente da última que enviamos
    if (p_str != ultimo_conteudo_enviado) {
        cout << "[LOG] Mensagem Enviada:\n" << p_str << "-------------------" << endl;
        ultimo_conteudo_enviado = p_str; // Atualiza o estado
    }

    //cout << "[LOG] Mensagem Enviada:\n" << p_str << "-------------------" << endl;
}

// Função para conectar ao gerenciador e realizar o Handshakeobrigatório
int conectar_e_autenticar(const string& ip, int porta, const string& funcao) {
    // Cria o socket TCP do cliente
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erro ao criar socket do cliente");
        exit(1);
    }

    // Configura o endereço do servidor (Gerenciador)
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porta);
    
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "Endereço IP inválido." << endl;
        exit(1);
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro ao conectar ao Gerenciador");
        exit(1);
    }

    cout << "[CONEXÃO] Conectado ao Gerenciador. Enviando Handshake..." << endl;

    // Monta o REQ_CON 
    req_con handshake;
    strncpy(handshake.ip, ip.c_str(), sizeof(handshake.ip) - 1);
    strncpy(handshake.funcao, funcao.c_str(), sizeof(handshake.funcao) - 1);

    // Envia o pacote de Handshake para o servidor
    enviar_pacote(sock, handshake);

    // Aguarda a resposta CON_ACK do servidor
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        cerr << "Erro ou desconexão ao esperar CON_ACK." << endl;
        close(sock);
        exit(1);
    }

    cout << "[LOG] Resposta do Gerenciador recebida:\n" << buffer << "-------------------" << endl;
    
    // Uma validação simples se a conexão foi aceita (STATUS: SUCESS ou similar no buffer)
    if (string(buffer).find("TIPO_MSG: CON_ACK") == string::npos) {
        cerr << "Falha na autenticação com o Gerenciador." << endl;
        close(sock);
        exit(1);
    }

    cout << "[SUCESSO] Dispositivo autenticado como: " << funcao << "\n" << endl;
    return sock;
}

// === COMPORTAMENTOS DOS COMPONENTES ===

void manda_presenca_periodico(int sock, sens_presenca &presenca) {
	clock_t tempo_old = 0, tempo = 0;

	while (true) {
		tempo = clock();
		if ((tempo - tempo_old) / CLOCKS_PER_SEC >= 1) {
			enviar_pacote(sock, presenca);
			tempo_old = tempo;
		}
	}
}

void rodar_sensor_presenca(int sock) {
    cout << "=== MÓDULO: SENSOR DE PRESENÇA ===" << endl;
    cout << "Digite '1' para simular PRESENÇA DETECTADA ou '0' para simular PRESENÇA NÃO DETECTADA ou '2' para sair." << endl;

	sens_presenca presenca;
	presenca.detectado = false;

	thread manda_presenca([sock, &presenca]() { manda_presenca_periodico(sock, presenca); });
	manda_presenca.detach();

	// O sensor de presença fica em loop aguardando o 
    // usuário simular a detecção de presença.
	while (true) {
        string entrada;
        cin >> entrada;
        if (entrada == "1") {
            presenca.detectado = true;
        } else if (entrada == "0") {
            presenca.detectado = false;
        } else if (entrada == "2") {
			break;
		}
    }
}

void rodar_leitor_cartao(int sock) {
    cout << "=== MÓDULO: LEITOR DE CARTÃO ===" << endl;
    // O leitor de cartão fica em loop aguardando o 
    // usuário digitar o nome e número USP do aluno.
    while (true) {
        string nome;
        int numero;
        cout << "Digite o primeiro nome do aluno (ou 'sair'): ";
        cin >> nome;
        if (nome == "sair") break;
        cout << "Digite o número USP do aluno: ";
        cin >> numero;

        sens_cartao cartao;
        cartao.numero = numero;
        cartao.nome = nome;
        
        enviar_pacote(sock, cartao);
    }
}

void rodar_atuador(int sock, const string& nome_atuador) {
    cout << "=== MÓDULO ATUADOR: " << nome_atuador << " ===" << endl;
    cout << "Aguardando comandos do Gerenciador..." << endl;

    char buffer[1024];

    // O atuador fica em loop aguardando comandos do 
    // gerenciador para ligar ou desligar o dispositivo.
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_recados = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_recados <= 0) {
            cout << "Conexão perdida com o Gerenciador." << endl;
            break;
        }

        cout << "[COMANDO RECEBIDO]:\n" << buffer << "-------------------" << endl;
        
        // Aqui o atuador processa a string que o servidor mandou (ex: ACAO: LIGAR)
        string msg(buffer);
        if (msg.find("ACAO: LIGAR") != string::npos) {
            cout << "[AÇÃO] " << nome_atuador << " FOI LIGADO(A)!" << endl;
        } else if (msg.find("ACAO: DESLIGAR") != string::npos) {
            cout << "[AÇÃO] " << nome_atuador << " FOI DESLIGADO(A)!" << endl;
        }
    }
}

void rodar_cliente_professor(int sock) {
    cout << "=== MÓDULO: REQUISIÇÃO DO PROFESSOR ===" << endl;
    get_presenca requisicao;
    enviar_pacote(sock, requisicao);

    // Recebe a lista de presença de volta
    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    int bytes_recados = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_recados > 0) {
        cout << "=== LISTA DE PRESENÇA RECEBIDA ===" << endl;
        cout << buffer << endl;
    }
}

int main(int argc, char* argv[]) {
    // Verifica se os argumentos necessários foram fornecidos
    if (argc < 4) {
        cout << "Uso correto: ./client [IP_GERENCIADOR] [PORTA] [TIPO]" << endl;
        cout << "Tipos validos: PRESENCA, CARTAO, CHAVE, ILUMINACAO, PROJETOR, AR_CONDICIONADO, PROFESSOR" << endl;
        return 1;
    }

    // Lê os argumentos de linha de comando
    string ip = argv[1];
    int porta = stoi(argv[2]);
    string tipo = argv[3];

    // Conecta e faz o handshake automaticamente
    int sock = conectar_e_autenticar(ip, porta, tipo);

    // Desvia o fluxo para o comportamento do componente escolhido
    if (tipo == "PRESENCA") {
        rodar_sensor_presenca(sock);
    } else if (tipo == "CARTAO") {
        rodar_leitor_cartao(sock);
    } else if (tipo == "ILUMINACAO" || tipo == "PROJETOR" || tipo == "AR_CONDICIONADO") {
        rodar_atuador(sock, tipo);
    } else if (tipo == "PROFESSOR") {
        rodar_cliente_professor(sock);
    } else {
        cout << "Tipo de dispositivo desconhecido." << endl;
    }

    close(sock);
    cout << "Conexão encerrada." << endl;
    return 0;
}
