TARGET=client server 
CC=g++
CFLAGS= -Wall -Wextra -g
normal: $(TARGET)
client: client.cpp
	$(CC) client.cpp -o client
server: server.cpp
	$(CC) server.cpp -o server
clean:
	$(RM) $(TARGET)
