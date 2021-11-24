#!/bin/bash
set -e
td=$(mktemp -d)
of=$td/stdout
QTY=119

if ! ( (./bootstrap.sh && ./configure && make clean && make) 2>&1 |pv -s $QTY -l -N "Compiling") >$of ; then
  cat $of
else
  ./test.sh
fi

rm -rf $td
