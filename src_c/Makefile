CC=gcc
CFLAGS=-O2 -Wall -Wextra -Iinclude -std=c11
LDFLAGS=

BIN_DIR=bin
BIN=$(BIN_DIR)/traffic_sim

SRC=main.c config_kv.c rng.c grid.c controllers.c stats.c sim.c
OBJ=$(SRC:.c=.o)

all: $(BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN): $(BIN_DIR) $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ)
	rm -rf $(BIN_DIR)
