# 컴파일러
CXX := g++

# 공통 옵션
INCLUDES := -I../../../resources/userdata/external -I../../bsw/ipc/inc -I../../bsw/ipc/lib
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 $(INCLUDES) $(shell pkg-config --cflags Qt6Widgets)
LDFLAGS := $(shell pkg-config --libs Qt6Widgets)

# 타겟 실행 파일 이름
TARGET := app

# 소스 파일
SRCS := main.cpp UserData.cpp BaseData.cpp ImageData.cpp RenderingData.cpp InfotainmentWidget.cpp AppController.cpp ShmCompat.cpp

# 오브젝트 파일
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
