# SmartClass v1.0 - Sistema de Automação de Sala de Aula

Este projeto consiste em um sistema distribuído em C++ utilizando a API de **Sockets TCP (Internet Stream)** e **Múltiplas Threads** para simular e gerenciar a automação de uma sala de aula inteligente baseada no protocolo textual próprio `SMARTCLASS/1.0`.

## Integrantes do Grupo
* **[Lucas Rodrigues Baptista]** - Nº USP: `[15577631]`
* **[João Victor De Bortoli Prado]** - Nº USP: `[13672071]`

---

## Descrição do Projeto

O sistema é dividido em duas partes principais que se comunicam através da rede:

1. **Gerenciador (Servidor):** Um servidor multi-thread centralizado que escuta em uma porta específica. Para cada dispositivo (cliente) que se conecta, o gerenciador realiza um *handshake* de autenticação e dispara uma thread dedicada em modo **bloqueante (blocking)** para monitorar as mensagens daquele cliente de forma assíncrona, utilizando **semáforos** para proteger as regiões críticas de dados compartilhados (como a lista de chamada).
2. **Clientes (Dispositivos):** Um executável versátil capaz de simular diferentes entidades da sala através de parâmetros, divididos em:
   * **Sensores:** Enviam eventos ao servidor (Presença detectada, Chave seletora ativada, Cartão de estudante lido).
   * **Atuadores:** Dispositivos passivos que ficam aguardando comandos do Gerenciador para alterar seu estado (Iluminação, Ar Condicionado, Projetor).
   * **Professor:** Um cliente de consulta rápida que solicita o relatório da lista de chamadas atual do servidor e encerra.

---

## Pré-requisitos

O projeto foi desenvolvido para ambientes baseados em **Linux** (POSIX) e requer:
* Compilador `g++` com suporte a **C++11** ou superior.
* Utilitário `make`.
* Biblioteca de threads nativa (`pthread`).

---

## Como Compilar e Executar

Para testar o ecossistema completo, recomenda-se a utilização de **múltiplas abas ou janelas de terminais** paralelos.

### 1. Compilação
Na raiz do projeto, execute o comando abaixo para compilar o Gerenciador e o Cliente:
```bash
make

### 2. Inicializando o Servidor
No primeiro terminal, inicialize o servidor central na porta padrão (configurada no Makefile como 6767):
```bash
make run

### 3. Conectando os Atuadores (obrigatório antes dos sensores):
Abra novos terminais e conecte os atuadores da sala para que eles fiquem prontos para receber comandos do gerenciador:
```bash
./client.out 127.0.0.1 6767 ILUMINACAO
./client.out 127.0.0.1 6767 AR_CONDICIONADO
./client.out 127.0.0.1 6767 PROJETOR

### 4. Interagindo com os Sensores:
Com a sala estruturada, use novos terminais para simular eventos físicos e observar as regras de automação disparadas no servidor e replicadas nos atuadores:
```bash
./client.out 127.0.0.1 6767 PRESENCA
./client.out 127.0.0.1 6767 CARTAO
./client.out 127.0.0.1 6767 CHAVE

### 5. Módulo do Professor:
A qualquer momento, para extrair a lista de alunos presentes que passaram o cartão, execute em um terminal separado:
```bash
./client.out 127.0.0.1 6767 PROFESSOR

## Limpeza dos Arquivos
Para remover os arquivos compilados (.out) e limpar o diretório de trabalho, utilize:
```bash
make clean