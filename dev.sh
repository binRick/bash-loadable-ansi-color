nodemon -w src -e c,sh -x sh -- -c 'make clean >/dev/null 2>&1; make >/dev/null && ./test.sh||true'
