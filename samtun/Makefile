FLAGS=-Wall -std=c++11 -static
SRC=base64.cc sha2.c address.cc dht.cc
OBJS=base64.o sha2.o address.o dht.o
CXX=g++
#CXX=clang++

all: samtun.bin

%.o: %.cc $(SRC)
	$(CXX) -c $< -o $@ $(FLAGS)

samtun.bin: $(OBJS)
	$(CXX) $(FLAGS) samtun.cc $^ -o samtun.bin

clean:
	rm -f samtun.bin *.o

test: $(OBJS)
	$(CXX) test_dht.cc -o test_dht $(OBJS) $(FLAGS)

