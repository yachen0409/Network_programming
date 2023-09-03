TARGET=server 
CC=g++
CFLAGS= -Wall -Wextra -g
normal: $(TARGET)
server: server.cpp
	$(CC) server.cpp -o build/server --std=c++17
clean:
	$(RM) $(TARGET)
