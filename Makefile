CC=g++
FLAGS=--std=c++11
PORTA=6767

.PHONY: clean run

gerenciador.out: server.cpp server.hpp protocol.hpp gerenciador.cpp
	$(CC) gerenciador.cpp server.cpp -o gerenciador.out

run:
	./gerenciador.out $(PORTA)
