#!/bin/bash
set -e
clear
BASH_PREFIX="/opt/bash-5.1/bin/bash --norc --noprofile"
do_test() {
	cmd="$1"
	txt="${2:-$(date)}"
	cmd="$BASH_PREFIX -c 'enable -f ./libs/color.so color && { eval \"$cmd\" && echo -e \"$txt\"; color bg reset; }; { color fg off && color bg off; }'"
	eval "$cmd"
}

do_test "color usage"
do_test "color -h"
do_test "color --help"
do_test "color --list"
do_test "color -c green && color --strike" "green and strike"
do_test "color --inverse" "inverse"
do_test "color fg inverse" "inverse"
do_test "color fg strike" "strike"
do_test "color fg blink" "blink"
do_test "color fg rapidblink" "rapidblink"
do_test "color fg dim" "dim text"
do_test "color fg dim && color fg ul" "dim and underlined"
do_test "color --italic" "italic text"

for b in black white; do
	for c in yellow red; do
		do_test "color fg $c" "$c text"
		do_test "color fg lt$c" "light $c text"
		do_test "color fg i$c" "intense $c text"
		do_test "color fg $c && color bg $b" "$c text on $b"
	done
done

do_test "color --bold" "bold text"
do_test "color --faint" "faint text"
do_test "color fg hide" "hidden text"
do_test "color fg strike" "striked"


do_test "color fg black && color bg yellow" "black/yellow"
do_test "color fg white && color bg yellow" "white/yellow"

#/opt/bash-5.1/bin/bash --norc --noprofile -c "enable -f ./libs/color.so color && color -c blue && date && color off && date && color -b green && date && color off && date && date && color -c red && color -b black && date && color off && date && color fg bd && date && color fg off && date && color fg bd && color --color blue && date && color fg off && color bg black && color fg cyan && color fg italic && date && color bg off && color fg blink && date"
