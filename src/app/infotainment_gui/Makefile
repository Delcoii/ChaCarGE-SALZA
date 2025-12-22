# 컴파일러
CXX := g++

# 컴파일 옵션
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iexternal $(shell pkg-config --cflags Qt6Widgets)
LDFLAGS := $(shell pkg-config --libs Qt6Widgets)
# 타겟 실행 파일 이름
TARGET := app

# 소스 파일
SRCS := main.cpp UserData.cpp BaseData.cpp ImageData.cpp

# 오브젝트 파일 (.cpp -> .o)
OBJS := $(SRCS:.cpp=.o)

# 기본 타겟
all: $(TARGET)

# 링크 단계
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 컴파일 단계
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 정리
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

