# Makefile for test_data_generator

# Configuration compiler
CC 			= gcc
CFLAGS 		= -Wall -g

# paths
# current directory : etc/test_data_generator
# root directory    : ../../
ROOT_DIR 	= ../../../../..
IPC_DIR 	= $(ROOT_DIR)/src/bsw/ipc

# include files directory and library directory
INC_DIR 	= $(IPC_DIR)/inc
LIB_DIR 	= $(IPC_DIR)/lib

# include and library options
INCLUDES 	= -I$(INC_DIR) -I$(LIB_DIR)
LIBS		= -lrt -lm

# build target
TARGET 		= can_rx_process

# source files
# 1) main source file
# 2) shared memory library
SRCS		= can_rx_process.c \
			  $(LIB_DIR)/shm_functions.c \
			  can_db_interface.c

# Rules of build
all: $(TARGET)

$(TARGET): $(SRCS)
		$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
		@echo "[Generator make file] Build completed: $(TARGET)"

clean:
		rm -f $(TARGET)
		@echo "[Generator make file] Clean completed."