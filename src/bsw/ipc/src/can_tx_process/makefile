# Makefile for can_tx_process

# Configuration compiler
CC      = gcc
CFLAGS  = -Wall -g

# paths
# current directory : src/bsw/ipc/src/can_tx_process
# root directory    : ../../../../..
ROOT_DIR = ../../../../..
IPC_DIR  = $(ROOT_DIR)/src/bsw/ipc

# include files directory and library directory
INC_DIR = $(IPC_DIR)/inc
LIB_DIR = $(IPC_DIR)/lib

# include and library options
INCLUDES = -I$(INC_DIR) -I$(LIB_DIR)
LIBS     = -lrt -lm

# build target
TARGET = can_tx_process

# source files
SRCS = can_tx_process.c

# Rules of build
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "[CAN TX make file] Build completed: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "[CAN TX make file] Clean completed."
