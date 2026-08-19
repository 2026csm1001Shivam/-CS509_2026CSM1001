CXX      := g++
CXXFLAGS := -O2 -std=c++11 -Wall -Wextra

TARGET := wrapper

ifeq ($(OS),Windows_NT)
RM = del /q
EXE = .exe
else
RM = rm -f
EXE =
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): wrapper.cpp
	$(CXX) $(CXXFLAGS) wrapper.cpp -o $(TARGET)

clean:
	$(RM) $(TARGET)$(EXE)