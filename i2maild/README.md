# i2maild

smtp server implementation for i2p

## building 

from git repo:

    git clone https://github.com/majestrate/i2p-tools/
    cd i2p-tools/i2maild
    ./build.sh


with `go get`:

    go get -u github.com/majestrate/i2p-tools/i2maild/cmd/i2maild

## usage:

to run the smtp server:

    i2maild -config mailserver.json -logtostderr


use 127.0.0.1:7625 as smtp server to send email to other .b32.i2p domains that you KNOW run a smtp server

my test email is psi@tvzz6sfrup7xwshkwya7l7kmurvv6nhahozrpsvb55ivlkqwhsvq.b32.i2p

all inbound mail will be delivered into ./i2maild in maildir format by default
