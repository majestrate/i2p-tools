FLAGS=-Wall -std=c++11
SRC=base64.cc samtun.cc sha2.c address.cc dht.cc
OBJS=base64.o samtun.o sha2.o address.o dht.o

all: samtun.bin

%.o: %.cc $(SRC)
	g++ -c $< -o $@ $(FLAGS)

samtun.bin: $(OBJS)
	g++ -o samtun.bin $^

clean:
	rm -f samtun.bin *.o

