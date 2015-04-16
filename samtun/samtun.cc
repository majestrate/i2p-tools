//
// samtun.cc
// copywrong you're mom 2015
//
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <signal.h>
#include <cstring>
#include <thread>
#include <memory>
#include "address.hpp"
#include "dht.hpp"

extern "C" {
#include "ifr6.h"
}

#define ASSERT_FD(fd) { if (fd == -1) { std::string msg(__FILE__); msg += ":" + std::to_string(__LINE__); perror(msg.c_str()); exit(1); } }

#define CLOSE_IF_OPEN(fd) { if ( fd != -1 ) { close(fd); } }

#define SAM_PRIVKEY_FILE "privkey.txt"

#define MAX_DESTINATION_LEN 1024

namespace samtun {
  
  bool verbose = false;
  bool _ready = false;

  // sam , datagram , tun devicem descriptors
  int cmdfd, dgramfd, tunfd;

  // tunnel name
  std::string samnick;

  // tun device network interface name
  std::string iface_name;
  
  // private key stuff
  std::string privkey;
  std::string privkey_fname;

  // our destination
  std::string our_dest;
  
  // address to send datagrams
  sockaddr_in sam_dgram_addr;
  // address to control sam
  sockaddr_in sam_cmd_addr;


  // their tun address
  in_addr_t them_addr;
  // our tun address
  in_addr_t us_addr;
  
  // network mtu
  size_t mtu;

  // dht handler
  DHT_t dht;

  // does our keyfile exist?
  bool has_keyfile() {
    std::fstream f;
    f.open(privkey_fname);
    return f.is_open();
  }


  // dump private key to file
  void dump_privkey() {
    std::fstream f;
    f.open(privkey_fname, std::ios::out);
    f.write(privkey.c_str(), privkey.length());
  }

  // send line to sam
  void sam_cmd_writeline(std::string line) {
    line += "\n";
    if ( verbose ) {
      std::cerr << "sam write: " << line;
    }
    send(cmdfd, line.c_str(), line.length(), 0);
  }

  // read line from sam
  std::string sam_cmd_readline() {
    std::string line;
    char c;
    while(true) {
      recv(cmdfd, &c, sizeof(c), 0);
      if ( c == '\n' ) {
        break;
      }
      line += c;
    }
    if (verbose) {
      std::cerr << "sam read: " << line << std::endl;
    }
    return line;
  }

  // open network interface
  void open_tun() {
    tunfd = open("/dev/net/tun", O_RDWR);
    ASSERT_FD(tunfd);

    ifreq ifr;
    memset(&ifr, 0, sizeof(ifreq));
    // set interface name
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, iface_name.c_str(), IFNAMSIZ);
    if ( ioctl(tunfd, TUNSETIFF, &ifr) < 0 ) {
      perror("TUNSEFIFF");
      exit(1);
    }

    // open socket for setting address, mtu, etc
    int sfd6 = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
    // get our interface's index
    if (ioctl(sfd6, SIOCGIFINDEX, &ifr) < 0 ) {
      perror("SIOCGIFINDEX6");
      exit(1);
    }
    
    in6_ifreq ifr6;
    memset(&ifr6, 0, sizeof(ifr6));
    ifr6.ifr6_prefixlen = 8;
    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    memcpy(&ifr6.ifr6_addr, &dht.node_addr, sizeof(in6_addr));
    
    if (ioctl(sfd6, SIOCSIFADDR, &ifr6) < 0 ) {
      perror("SIOCSIFADDR6");
      exit(1);
    }
    // set mtu
    ifr.ifr_mtu = mtu;
    if (ioctl(sfd6, SIOCSIFMTU, &ifr) < 0) {
      perror("SIOCSIFMTU4");
      exit(1);
    }

    // get flags
    if (ioctl(sfd6, SIOCGIFFLAGS, &ifr) < 0) {
      perror("SIOCGIFFLAGS");
      exit(1);
    }

    // set interface flags as up
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(sfd6, SIOCSIFFLAGS, &ifr) < 0) {
      perror("SIOCSIFFLAGS");
      exit(1);
    }
    // close sockets
    close(sfd6);
  }

  // load private keys
  void load_keys() {
    std::ifstream f;
    f.open(privkey_fname);
    if (!f.is_open()) {
      std::cerr << "failed to open private key file: " << privkey_fname;
      std::cerr << std::endl;
      exit(1);
    }
    // get length of file:
    f.seekg(0, f.end);
    int length = f.tellg();
    f.seekg(0, f.beg);
    // pad privatkey
    privkey.resize(length, ' ');
    // get privatekey
    auto itr = &*privkey.begin();
    f.read(itr, length);
    f.close();
  }

  // find a privatekey and save it to file
  void extract_and_save_key(std::string line) {
    if (line.find("RESULT=OK") == std::string::npos ) {
      std::cout << "failed to open sam session: ";
      std::cout << line;
      std::cout << std::endl;
      exit(1);
    }
    auto pos = line.find("DESTINATION=");
    if (pos != std::string::npos ) {
      auto part = line.substr(pos+12);
      pos = part.find(" ");
      if ( pos == std::string::npos ) {
        privkey = part;
      } else {
        privkey = part.substr(0, pos);
      }
    }
    dump_privkey();
  }

  // given a name lookup response get the destination
  // return empty string on error
  std::string get_name_lookup_result(std::string line) {
    auto pos = line.find("RESULT=OK");
    if (pos == std::string::npos ) {
      return "";
    }
    pos = line.find("VALUE=");
    auto part = line.substr(pos+6);
    pos = part.find(" ");
    if ( pos == std::string::npos ) {
      return part;
    }
    return part.substr(0, pos);
  }

  // initialize state
  void init() {
    _ready = false;
    std::string sverb = std::getenv("SAM_VERBOSE");
    if (sverb == "yes") {
      std::cerr << "verbose output on" << std::endl;
      verbose = true;
    }
    samnick = std::getenv("SAM_NICK");
    std::string sam_cmd_host = std::getenv("SAM_CMD_HOST");
    std::string sam_cmd_port = std::getenv("SAM_CMD_PORT");
    std::string sam_udp_host = std::getenv("SAM_UDP_HOST");
    std::string sam_udp_port = std::getenv("SAM_UDP_PORT");
    std::string sam_recv_host = std::getenv("SAM_RECV_HOST");
    std::string sam_recv_port = std::getenv("SAM_RECV_PORT");
    iface_name = std::getenv("SAM_TUN_IFACE");
    
    privkey_fname = std::getenv("SAM_KEYFILE");
    
    if (verbose) {
      std::cerr << "nickname   " << samnick << std::endl;
      std::cerr << "iface name " << iface_name << std::endl;
      std::cerr << "keyfile    " << privkey_fname << std::endl;
      std::cerr << "cmd host   " << sam_cmd_host << std::endl;
      std::cerr << "cmd port   " << sam_cmd_port << std::endl;
      std::cerr << "udp host   " << sam_udp_host << std::endl;
      std::cerr << "udp port   " << sam_udp_port << std::endl;
      std::cerr << "recv host  " << sam_recv_host << std::endl;
      std::cerr << "recv port  " << sam_recv_port << std::endl;
    }

    // check nickname set
    if ( samnick.length() == 0 ) {
      std::cerr << "no tunnel name specified";
      std::cerr << std::endl;
      exit(1);
    }

    // check keyfile set
    if ( privkey_fname.length() == 0 ) {
      std::cerr << "no keyfile specified";
      std::cerr << std::endl;
      exit(1);
    }

    // set command port
    int cmd_port = std::atoi(sam_cmd_port.c_str());
    if (cmd_port == -1) {
      std::cerr << "bad sam port " << sam_cmd_port;
      std::cerr << std::endl;
      exit(1);
    }
    
    // set udp port
    int udp_port = std::atoi(sam_udp_port.c_str());
    if (udp_port == -1) {
      std::cerr << "bad sam udp " << sam_udp_port;
      std::cerr << std::endl;
      exit(1);
    }
    
    // set recv port
    int recv_port = std::atoi(sam_recv_port.c_str());
    if ( recv_port == -1 ) {
      std::cerr << "bad recv port " << sam_recv_port;
      std::cerr << std::endl;
      exit(1);
    }

    // set mtu
    mtu = 8 * 1024;

    // set command address
    sam_cmd_addr.sin_family = AF_INET;
    sam_cmd_addr.sin_port = htons(cmd_port);
    sam_cmd_addr.sin_addr.s_addr = inet_addr(sam_cmd_host.c_str());

    // set udp address
    sam_dgram_addr.sin_family = AF_INET;
    sam_dgram_addr.sin_port = htons(udp_port);
    sam_dgram_addr.sin_addr.s_addr = inet_addr(sam_udp_host.c_str());

    // set udp recv address
    sockaddr_in sam_udp_bind_addr;
    sam_udp_bind_addr.sin_family = AF_INET;
    sam_udp_bind_addr.sin_port = htons(recv_port);
    sam_udp_bind_addr.sin_addr.s_addr = inet_addr(sam_recv_host.c_str());

    // open command socket
    cmdfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_FD(cmdfd);

    if (connect(cmdfd, (const sockaddr *) &sam_cmd_addr, sizeof(sam_cmd_addr))) {
      perror("Cannot connect to SAM");
      exit(1);
    }

    
    // open udp socket
    dgramfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_FD(dgramfd);

    if (bind(dgramfd, (const sockaddr *) &sam_udp_bind_addr, sizeof(sam_udp_bind_addr))) {
      perror("dgramfd::bind()");
      exit(1);
    }
    
    sam_cmd_writeline("HELLO VERSION MIN=3.0 MAX=3.0");

    auto line = sam_cmd_readline();
    
    std::stringstream ss;
    ss << "SESSION CREATE STYLE=DATAGRAM ID=";
    ss << samnick << " DESTINATION=";
    if ( has_keyfile() ) {
      load_keys();
      ss << privkey;
    } else {
      ss << "TRANSIENT";
    }
    ss << " HOST=" << sam_recv_host;
    ss << " PORT=" << sam_recv_port;

    // create destination
    sam_cmd_writeline(ss.str());
    // read response
    line = sam_cmd_readline();
    // save key as needed
    extract_and_save_key(line);

    // zero out private key
    privkey = "";

    // lookup our own i2p destination
    sam_cmd_writeline("NAMING LOOKUP NAME=ME");
    line = sam_cmd_readline();
    our_dest = get_name_lookup_result(line);


    // set the dht up
    dht.Init(our_dest);
    
    // restore our nodes
    dht.Restore("nodes.txt");
    
    // compute our node's ipv6 address
    i2p_b32addr_t addr(our_dest);
    
    std::cout << "our ipv6 address is " << addr_tostring(dht.node_addr);
    std::cout << std::endl;
    
    std::cout << "our destination is: " << our_dest;
    std::cout << std::endl;
    _ready = true;
  }
  
  bool ready() {
    return _ready;
  }

  int writetun(void * buff, size_t bufflen) {
    return write(tunfd, buff, bufflen);
  }
  
  // send a datagram to sam udp
  int sam_sendto(std::string addr, void * buff, size_t bufflen) {
    std::stringstream ss;
    ss << "3.0 " << samnick << " " << addr << "\n";
    std::string header = ss.str();
    size_t sendlen = header.size() + bufflen;
    char * send_buff = new char[sendlen];
    
    memcpy(send_buff, header.c_str(), header.size());
    memcpy(send_buff+header.size(), buff, bufflen);
    
    if (verbose) {
      std::cerr << "sam_sendto " << std::to_string(sendlen) << " " << addr;
      std::cerr << std::endl;
    }
    
    int res = sendto(dgramfd, send_buff, sendlen, 0, (const sockaddr *)&sam_dgram_addr, sizeof(sam_dgram_addr));
    delete send_buff;
    return res;
  }
  
  // we got a sam datagram from addr 
  void got_sam_datagram(std::string addr, char * buff, size_t bufflen) {
    if (verbose) {
      std::cerr << "got a datagram from " << addr << " " << std::to_string(bufflen) << "B";
      std::cerr << std::endl;
    }

    // compute b32 address
    i2p_b32addr_t fromaddr(addr); 

    
    // if we don't already know this destination put it in our routing table
    if (!dht.KnownDest(addr)) {
      if (verbose) {
        std::cerr << "put " << addr << " into routing table" << std::endl;
      }
      dht.Put(addr);
    }

    // check if this is a control packet
    uint8_t version;
    memcpy(&version, buff, 1);
    // is it a dht packet?
    if (!version) {
      // yes
      // handle dht packet
      std::cerr << "handle dht packet" << std::endl;
      dht.HandleData(addr, buff, bufflen, sam_sendto);
    } else {
      // it's an ipv6 packet
      
      // get destination address
      in6_addr dst;
      memcpy(&dst, buff+28, 16);

      // is this packet for us?
      if (!bcmp(&dst, &dht.node_addr, 16)) {
        // no it's not wtf?
        // drop it
        return;
      }
      
      // get source address
      in6_addr src = fromaddr;
      // put it into the packet
      memcpy(buff+10, &src, 16);
      std::cerr << "write tun " << bufflen << std::endl;
      // write packet to the tun device;
      writetun(buff, bufflen);
    }
  }

  // called when we got an ip packet from the user
  void got_ip_packet(char * buff, size_t bufflen) {
    std::cerr << "got ip packet" << std::endl;
    if(bufflen <= 36) {
        std::cerr << "packet too small " << std::to_string(bufflen);
        std::cerr << std::endl;
        // packet too small
      return;
    }
    // get the remote destination for this packet's destination ip
    in6_addr dst;
    memcpy(&dst, buff+28, 16);
    // do we know this address ?
    if ( dht.KnownAddr(dst)) {
      // yes, send it to them
      std::cerr << "sending to " << addr_tostring(dst) << std::endl;
      std::string dest = dht.GetDest(dst);
      sam_sendto(dest, buff, bufflen);
    } else {
      // nope, we don't know who it's for. look it up.
      
      std::cerr << "finding " << addr_tostring(dst) << "..." << std::endl;
      
      dht.Find(dst, sam_sendto);
    } 
  }

  // called when we got a datagram from the i2p router
  void got_dgram(char * buff, size_t bufflen) {
    // find the newline
    char * payload = buff;
    while(*payload != '\n') { payload++; }

    // replace the newline with a null character
    // this terminates the string right there so we can...
    *payload = 0;
    // ... extract the source destination
    std::string from_dest(buff);

    // payload will point to the payload of the datagram
    ++payload;
    // adjust the length to fit the payload
    bufflen -= from_dest.size() + 1;
    // we got a datagram from the i2p router, bonzai!!
    got_sam_datagram(from_dest, payload, bufflen);
  }
  
  void run() {
    // open tun interface
    open_tun();

    size_t tun_buff_len = mtu + sizeof(iphdr);
    size_t dgram_buff_len = tun_buff_len + MAX_DESTINATION_LEN + 1;
    char dgram_buff[dgram_buff_len];
    char tun_buff[tun_buff_len];

    while(_ready) {
      if (verbose) {
        std::cerr << "select...";
      }
      // set up select
      int res;
      int nfds = std::max(tunfd, dgramfd);
      fd_set rd, wr, er;
      FD_ZERO(&er);
      FD_ZERO(&rd);
      FD_ZERO(&wr);
      FD_SET(tunfd, &rd);
      FD_SET(dgramfd, &rd);

      // select
      res = select(nfds + 1, &rd, &wr, &er, nullptr);
      if (res == -1) {
        perror("select()");
        exit(1);
      }
      if (verbose) {
        std::cerr << std::endl;
      }
      
      // tun device can read
      if (FD_ISSET(tunfd, &rd)) {
        // read a packet we will send
        int read_amount = read(tunfd, tun_buff, tun_buff_len);
        if (read_amount <= 0) {
          perror("tunfd::read()");
          exit(1);
        }
        // we got an ip packet
        got_ip_packet(tun_buff, read_amount);
      }

      // datagram socket can read
      if (FD_ISSET(dgramfd, &rd)) {
        sockaddr_in dgram_src;
        socklen_t dgram_src_len;
        // recv last datagram
        int read_amount = recvfrom(dgramfd, dgram_buff, dgram_buff_len, 0, (sockaddr *)&dgram_src, &dgram_src_len);
        if (read_amount <= 0) {
          perror("dgramfd::read()");
          exit(1);
        }
        // is this from the i2p router?
        if (dgram_src.sin_addr.s_addr == sam_dgram_addr.sin_addr.s_addr) {
          // yes, handle it
          got_dgram(dgram_buff, read_amount);
        } else {} // no. unwarrented probe?
      }
    }
  }
}

int main(int argc, char * argv[]) {
  samtun::init();
  if( samtun::ready()) {
    samtun::run();
  }
}
