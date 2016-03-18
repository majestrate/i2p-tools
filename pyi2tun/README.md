# pyi2tun #

point to point ip over i2p+udp shim

## usage ##

    # set up dependancies
    pyvenv venv
    venv/bin/pip install -r requirements.txt
    
    # generate initial configuration
    sudo venv/bin/python -m i2tun --generate

    # run vpn tunnel
    sudo venv/bin/python -m i2tun 
