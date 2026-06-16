CC=g++
FLAGS=--std=c++11 -g
PORTA=6767
IP=127.0.0.1

.PHONY: clean run

# Alvo padrão compila os dois binários
all: gerenciador.out client.out

gerenciador.out: server.cpp server.hpp protocol.hpp gerenciador.cpp
	$(CC) $(FLAGS) gerenciador.cpp server.cpp -o gerenciador.out

client.out: client.cpp protocol.hpp
	$(CC) $(FLAGS) client.cpp -o client.out

run: gerenciador.out
	./gerenciador.out $(PORTA)

clean:
	rm -f gerenciador.out client.out