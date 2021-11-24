#!/bin/bash
set -e
(./bootstrap.sh && ./configure && make) |pv -s 102 -l -N "Compiling" >/dev/null
./test.sh
