#!/bin/bash
export GOPATH=$PWD/go
mkdir -p $GOPATH
go get -u github.com/majestrate/i2p-tools/samtun
cp $GOPATH/bin/samtun $PWD/samtund
