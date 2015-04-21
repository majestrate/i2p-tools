#include "dht.hpp"
#include <iostream>

int main(int argc, char * argv[]) {
  std::string dest_me("AAAABBBBSSSSFFFFYYYYAAAAGGGGAAAASSSSVVVVLLL0ppppAAAA");
  std::string dest_them("AAAABBBBSSSSFFFFYYYYAAAAGGGGAAAASSSSVVVVLLL0BBBBAAAA");
  DHT_t dht;
  dht.Init(dest_me);
  dht.Put(dest_them);
}
