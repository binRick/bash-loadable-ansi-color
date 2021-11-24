#!/bin/sh
command -v libtool || dnf -y install libtool
autoreconf --install
automake --add-missing --copy >/dev/null 2>&1
