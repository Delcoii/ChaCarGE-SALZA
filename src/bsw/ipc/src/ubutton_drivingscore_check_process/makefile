# Makefile for button_drivingscore_check_process

CC      = gcc
CFLAGS  = -Wall -g

ROOT_DIR = ../../../../..
IPC_DIR  = $(ROOT_DIR)/src/bsw/ipc

INC_DIR = $(IPC_DIR)/inc
LIB_DIR = $(IPC_DIR)/lib

INCLUDES = -I$(INC_DIR) -I$(LIB_DIR)
LIBS     = -lwiringPi -lrt -lpthread -lm

TARGET = button_drivingscore_check_process

SRCS = button_drivingscore_check_process.c \
       $(LIB_DIR)/shm_functions.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "[Button makefile] Build completed: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "[Button makefile] Clean completed."