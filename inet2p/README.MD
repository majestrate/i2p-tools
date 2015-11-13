## Inet2P

This repo contains all resources to join Inet2P

### What is Inet2P?

Inet2P is an anonymous fullmesh IP network inside i2p.

Currently Inet2P uses the 10.10.0.0/16 address range. This may change.

## Building samtun

requirements:

* go 1.3 or higher
* git
* i2p 0.9.22 or higher

build: via i2p:

    git config --global http.proxy http://127.0.0.1:4444/
    git clone http://em763732l4b7b7zhaolctpt6wewwr7zw3nsxfchr6qmceizzmgpa.b32.i2p/psi/i2p-tools.git
    cd i2p-tools/inet2p
	./build.sh

build via clearnet:

    torify git clone https://github.com/majestrate/i2p-tools
    cd i2p-tools/inet2p
	./build.sh
