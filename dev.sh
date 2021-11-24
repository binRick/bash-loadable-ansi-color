#!/bin/bash
set -e
DEFAULT_CMD="./test.sh"
cmd="${@:-$DEFAULT_CMD}"
cmd="nodemon -w \*.sh -w src -e c,sh,h --delay .2 -V -x /bin/sh -- -c 'make clean >/dev/null 2>&1; make >/dev/null && $cmd||true'"

eval "$cmd"
