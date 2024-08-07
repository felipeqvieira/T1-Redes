OBJ_DIR = objs
BIN_DIR = bin
SRC_DIR = src
LIB_DIR = lib
FLAGS = -Wall -Wextra -std=c99 -g
OBJS = connection.o command.o utils.o
OBJSDIR = $(addprefix $(OBJ_DIR)/, $(OBJS))
VPATH = $(SRC_DIR):$(LIB_DIR)

all: client server clean

debug: FLAGS += -DDEBUG -g
debug: all

client: client.o $(OBJS)
		gcc $(FLAGS) $(OBJ_DIR)/client.o  $(OBJSDIR) -o $(BIN_DIR)/client -lm

server: server.o $(OBJS)
		gcc $(FLAGS) $(OBJ_DIR)/server.o  $(OBJSDIR) -o $(BIN_DIR)/server -lm

client.o: client.c | $(OBJ_DIR) $(BIN_DIR)
		gcc $(FLAGS) -c $(SRC_DIR)/client.c -o $(OBJ_DIR)/client.o

server.o: server.c | $(OBJ_DIR) $(BIN_DIR)
		gcc $(FLAGS) -c $(SRC_DIR)/server.c -o $(OBJ_DIR)/server.o

connection.o: connection.h
		gcc $(FLAGS) -c $(SRC_DIR)/connection.c -o $(OBJ_DIR)/connection.o

command.o: command.h
		gcc $(FLAGS) -c $(SRC_DIR)/command.c -o $(OBJ_DIR)/command.o  -lm

utils.o: utils.h
		gcc $(FLAGS) -c $(SRC_DIR)/utils.c -o $(OBJ_DIR)/utils.o

$(OBJ_DIR) $(BIN_DIR) :
		mkdir -p $@

clean:
		-rm -rf $(OBJ_DIR) 
purge:
		-rm -rf $(OBJ_DIR) $(BIN_DIR)
remove:
		-rm *.mp4
compact:
		tar -czf lcmc20-fqv21.tar.gz $(SRC_DIR)/*.c $(LIB_DIR)/*.h Makefile Relatório.pdf