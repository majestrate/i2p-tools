#!/usr/bin/env bash
export SAM_CMD_HOST=127.0.0.1
export SAM_CMD_PORT=7656
export SAM_UDP_HOST=127.0.0.1
export SAM_UDP_PORT=7655
export SAM_RECV_HOST=127.0.0.1
export SAM_RECV_PORT=11100
export SAM_KEYFILE=privkey.txt
export SAM_NICK=samtun
export SAM_TUN_IFACE=i2p0
export SAM_TUN_MTU=4096
export SAM_TUN_US=10.100.0.1
export SAM_TUN_THEM=10.100.0.2
export SAM_VERBOSE=yes
gdb $(dirname $0)/samtun.bin 
