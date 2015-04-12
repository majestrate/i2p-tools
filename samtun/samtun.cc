//
// vpn tunnel for i2p via SAM 3
//
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
#include <cstring>
#include <thread>
#include <memory>

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

  // their remote destination
  std::string remote_dest;
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
    int sfd = socket(AF_INET, SOCK_DGRAM, 0);

    // set our address
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = us_addr;
    memcpy(&ifr.ifr_addr, &addr, sizeof(sockaddr_in));
    if (ioctl(sfd, SIOCSIFADDR, &ifr) < 0) {
      perror("SIOCSIFADDR");
      exit(1);
    }

    // set their address
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = them_addr;
    memcpy(&ifr.ifr_addr, &addr, sizeof(sockaddr_in));
    if (ioctl(sfd, SIOCSIFDSTADDR, &ifr) < 0) {
      perror("SIOCSIFDSTADDR");
      exit(1);
    }

    // set mtu
    ifr.ifr_mtu = mtu;
    if (ioctl(sfd, SIOCSIFMTU, &ifr) < 0) {
      perror("SIOCSIFMTU");
      exit(1);
    }

    // get flags
    if (ioctl(sfd, SIOCGIFFLAGS, &ifr) < 0) {
      perror("SIOCGIFFLAGS");
      exit(1);
    }

    // set interface flags as up
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(sfd, SIOCSIFFLAGS, &ifr) < 0) {
      perror("SIOCSIFFLAGS");
      exit(1);
    }
    // close socket for setting stuff
    close(sfd);
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
    std::string sam_us_addr = std::getenv("SAM_TUN_US");
    std::string sam_them_addr = std::getenv("SAM_TUN_THEM");
    std::string sam_net_mtu = std::getenv("SAM_TUN_MTU");
    
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
      std::cerr << "us addr    " << sam_us_addr << std::endl;
      std::cerr << "them addr  " << sam_them_addr << std::endl;
      std::cerr << "MTU        " << sam_net_mtu << std::endl;
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
    int net_mtu = std::atoi(sam_net_mtu.c_str());
    if ( net_mtu == - 1) {
      std::cerr << "bad mtu " << sam_net_mtu;
      std::cerr << std::endl;
      exit(1);
    }
    mtu = net_mtu;
    
    // set our / their tun addresses
    inet_pton(AF_INET, sam_us_addr.c_str(), &us_addr);
    inet_pton(AF_INET, sam_them_addr.c_str(), &them_addr);

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
    std::cout << "our destination is: " << our_dest;
    std::cout << std::endl;
    _ready = true;
  }
  
  bool ready() {
    return _ready;
  }

  // send a datagram to sam udp
  ssize_t sam_sendto(std::string addr, char * buff, size_t bufflen) {
    std::stringstream ss;
    ss << "3.0 " << samnick << " " << addr << "\n";
    std::string header = ss.str();
    size_t sendlen = header.size() + bufflen;
    char * send_buff = new char[sendlen];
    
    memcpy(send_buff, header.c_str(), header.size());
    memcpy(send_buff+header.size(), buff, bufflen);
    
    if (verbose) {
      std::cerr << "sam_sendto " << std::to_string(sendlen);
      std::cerr << std::endl;
    }
    
    int res = sendto(dgramfd, send_buff, sendlen, 0, (const sockaddr *)&sam_dgram_addr, sizeof(sam_dgram_addr));
    delete send_buff;
    return res;
  }

  // get in_addr for remote destination
  // return false if we don't know them
  bool get_address_for_remote(std::string remote, in_addr_t * addr) {
    if (remote == remote_dest) {
      *addr = them_addr;
      return true;
    }
    return false;
  }

  // resolve the base64 destination for an inet address
  // return empty string on failure
  std::string get_remote_for_address(in_addr_t addr) {
    if (addr == them_addr) {
      return remote_dest;
    } return "";
  }
  
  // get ip packet header from a buffer
  // return false if we can't
  bool get_ip_header(char * buff, size_t bufflen, iphdr * packet) {
    if (bufflen < sizeof(iphdr) ) {
      return false;
    }
    memcpy(&packet->saddr, buff+12, 4);
    memcpy(&packet->daddr, buff+16, 4);
    return true;
  }
  
  // we got a sam datagram from addr 
  void got_sam_datagram(std::string addr, char * buff, size_t bufflen) {
    if (verbose) {
      std::cerr << "got a datagram from " << addr << " " << std::to_string(bufflen) << "B";
      std::cerr << std::endl;
    }
    iphdr packet;
    // can we get the ip header out? is this remote destination known ?
    if (get_ip_header(buff, bufflen, &packet) && get_address_for_remote(addr, &packet.saddr)) {
      // put our address into the ip header as destination address
      memcpy(&packet.daddr, &us_addr, sizeof(packet.daddr));
      // put the header back onto the packet in the buffer
      // memcpy(buff, &packet, sizeof(iphdr));
      // write this to the tun device;
      write(tunfd, buff, bufflen);
    } else if (verbose) {
      std::cerr << "got unwarrented packet from " << addr;
      std::cerr << std::endl;
    }
  }

  // called when we got an ip packet from the user
  void got_ip_packet(char * buff, size_t bufflen) {
    
    if (verbose) {
      std::cerr << "got_ip_packet " << std::to_string(bufflen) << std::endl;
    }
    
    // get the remote destination for this packet's destination ip
    std::string dest = get_remote_for_address(them_addr);
    // do we know it?
    if (dest.size() > 0) {
      // yes, send it to them
      sam_sendto(dest, buff, bufflen);
    } else {} // nope, we don't know who it's for. drop it.  
  }

  // called when we got a datagram from the i2p router
  void got_dgram(char * buff, size_t bufflen) {
    if (verbose) {
      std::cerr << "got dgram " << std::to_string(bufflen);
      std::cerr << std::endl;
    }
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
    // is this a b32 or name ?
    // look it up
    if (remote_dest.find(".i2p") != std::string::npos ) {
      std::string name = remote_dest;
      do {
        std::cout << "looking up remote destination... ";
        std::stringstream ss;
        ss << "NAMING LOOKUP NAME=" << name;
        sam_cmd_writeline(ss.str());
        remote_dest = get_name_lookup_result(sam_cmd_readline());
      } while(remote_dest.size() == 0);
    }
    std::cout << "remote destination resolved: " << remote_dest;
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
  if( samtun::ready() && argc == 2) {
    samtun::remote_dest = argv[1];
    samtun::run();
  }
}
