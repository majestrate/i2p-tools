#!/usr/bin/env python
#
# i2pcontrol munin plugin
#
import sys
if len(sys.argv) > 1 and sys.argv[1] == 'config':
    print ('graph_title I2P Bandwidth')
    print ('graph_order down up')
    print ('graph_category network')
    print ('graph_vlabel bits in (-) / out (+) per ${graph_period}')
    print ('down.label bps')
    print ('down.type COUNTER')
    print ('up.label bps')
    print ('up.type COUNTER')
    print ('up.negative down')
else:
    import i2pcontrol
    ctl = i2pcontrol.I2PController(use_ssl=False)
    info = ctl.getRouterInfo()
    print ('down.value {}'.format(int(info['i2p.router.net.bw.inbound.1s'])))
    print ('up.value {}'.format(int(info['i2p.router.net.bw.outbound.1s'])))
