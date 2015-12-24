#!/bin/bash

set -e

root=$(dirname $(readlink -e $0))
repo=$(readlink -e $root/../)

export GOPATH=/tmp/go
mkdir -p $GOPATH/src/github.com/majestrate/i2p-tools/
cp -a $repo/{sam3,i2maild} $GOPATH/src/github.com/majestrate/i2p-tools/
cd $root
go get github.com/majestrate/i2p-tools/i2maild/cmd/i2maild
go install github.com/majestrate/i2p-tools/i2maild/cmd/i2maild
cp $GOPATH/bin/i2maild $root
