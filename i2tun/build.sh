#!/bin/bash
export GOPATH=$PWD/go
mkdir -p $GOPATH
go get -u github.com/majestrate/i2p-tools/i2tun
cp $GOPATH/bin/i2tun $PWD/i2tund
