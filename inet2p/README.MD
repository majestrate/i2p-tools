# Inet2P

This repo contains all resources to join Inet2P

## What is Inet2P?

Inet2P is an anonymous IP network inside i2p.

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


### Joining the network

Add your destination and address to network.json and submit a pull request

This entry would be the equivilent of saying ``"my destination is ne7d5elzpibgkk5qclusteavp5hc7avbl4p3yjtbjm5h4zkr2qvq.b32.i2p and I claim the address 10.10.55.6"``


    "ne7d5elzpibgkk5qclusteavp5hc7avbl4p3yjtbjm5h4zkr2qvq.b32.i2p": 10.10.55.6/32,


Currently whenever a change is made to network.json everyone must update their copy for it to have a global effect, this probably will change in the future.
