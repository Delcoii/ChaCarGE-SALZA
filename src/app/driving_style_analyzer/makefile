# Makefile for test_data_generator

# Configuration compiler
CC 			= gcc
CFLAGS 		= -Wall -g

# paths
# current directory : etc/test_data_generator
# root directory    : ../../
ROOT_DIR 	= ../../..
IPC_DIR 	= $(ROOT_DIR)/src/bsw/ipc
DSA_DIR     = $(ROOT_DIR)/src/app/driving_style_analyzer
BUILD_DIR	= $(ROOT_DIR)/build

# include files directory and library directory
INC_DIR 	= $(IPC_DIR)/inc
LIB_DIR 	= $(IPC_DIR)/lib

# include and library options
INCLUDES 	= -I$(INC_DIR) -I$(LIB_DIR)
LIBS		= -lrt -lm

# build target
TARGET 		= $(BUILD_DIR)/analyzer

# source files
# 1) main source file
# 2) shared memory library
SRCS		= analyzer.c \
			  $(LIB_DIR)/shm_functions.c \
			  $(DSA_DIR)/driving_score_calc.c

# Rules of build
all: $(TARGET)

$(TARGET): $(SRCS)
		@echo "[Analyzer] Checking build directory..."
		@mkdir -p $(dir $@) # Create build directory if it doesn't exist
		@echo "[Analyzer] Compiling: $@"
		$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
		@echo "[Analyzer] Build completed successfully!"

clean:
		@echo "[Analyzer] Cleaning..."
		rm -f $(TARGET)
		@echo "[Analyzer] Clean completed."