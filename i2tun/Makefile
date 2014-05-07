all_files = $(wildcard src/*.cpp)
FILES = $(all_files:%.cpp=%.o)
BIN_TARGET = i2tun
CXX = clang++
CXXFLAGS = -std=c++11 -Wall -I./include
LDFLAGS = `pkg-config --libs libi2cp`

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $(<:%.cpp=%.o)

all: clean $(FILES)
	$(CXX) -o $(BIN_TARGET) $(FILES) $(LDFLAGS)
clean:
	rm -f $(FILES) $(BIN_TARGET)
