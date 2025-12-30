# Compiler
CXX := g++
PKG_CONFIG ?= pkg-config

# Auto-select Qt version (prefer Qt6, fallback to Qt5)
QT_PKG := $(shell if $(PKG_CONFIG) --exists Qt6Widgets; then echo Qt6Widgets; elif $(PKG_CONFIG) --exists Qt5Widgets; then echo Qt5Widgets; fi)
ifeq ($(QT_PKG),)
$(error "Cannot find Qt Widgets package. Install qt6-base-dev (or qtbase5-dev) via apt.")
endif
QT_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(QT_PKG))
QT_LIBS := $(shell $(PKG_CONFIG) --libs $(QT_PKG))

# Common options
INCLUDES := -I../../../resources/userdata/external -I../../bsw/ipc/inc -I../../bsw/ipc/lib
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread $(INCLUDES) $(QT_CFLAGS)
LDFLAGS := $(QT_LIBS) -pthread -lrt

# Target executable name
TARGET := app

# Source files
SRCS := main.cpp UserData.cpp BaseData.cpp ImageData.cpp RenderingData.cpp InfotainmentWidget.cpp AppController.cpp ShmCompat.cpp

# Object files
OBJS := $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
