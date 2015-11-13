#!/bin/bash
set -e
# variables
absroot=$(readlink -e $(dirname $0))
export GOPATH=$absroot/go
srcroot=$GOPATH/src/github.com/majestrate/i2p-tools
gitroot=$(readlink -e $absroot/../)

# remove old binary
echo "clean..."
rm -rf $absroot/samtund
rm -rf $srcroot
# make source tree
mkdir -p $srcroot
# copy source
echo "copy source..."
cp -a $(readlink -e $absroot/../sam3) $(readlink -e $absroot/../samtun) $srcroot
# build daemon
echo "building samtund..."
go build -o $absroot/samtund github.com/majestrate/i2p-tools/samtun
