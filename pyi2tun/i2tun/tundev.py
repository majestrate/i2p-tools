#
# tun interface implementation for i2p.tun
#

import pytun
    
def opentun(cfg, tap=False):
    ifname  = cfg["ifname"]
    flag = pytun.IFF_TUN
    if tap:
        flag = pytun.IFF_TAP
    dev = pytun.TunTapDevice(ifname, flags=flag)
    dev.mtu = cfg["mtu"]
    if not tap:
        dev.addr = cfg["addr"]
        dev.netmask = cfg["netmask"]
    return dev
