#!/usr/bin/env bash
set -e
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PATH=$PATH:$(pwd)/bin
COLORS=0
DEFAULT_POST_CMD="color --list && color fg red && echo -n RED && color fg off && echo && echo OK"

test_builtin() {
	local M="$1"
	local N="$2"
	local post_cmd="${3:-$DEFAULT_POST_CMD}"
	local cmd="enable -f 'src/.libs/$M.so' $N && $N && $post_cmd && enable -d $N"
	cmd="command env command bash --norc --noprofile -c '$cmd'"

	err() {
		pfx="$(ansi --red --bold "$1")"
		msg="$(ansi --yellow --italic "$2")"
		if [[ "$COLORS" == 1 ]]; then
			echo -e "[$pfx]   $msg"
		else
			echo -e "[$1]   $2"
		fi
	}

	ok() {
		pfx="$(ansi --green --bold "$1")"
		msg="$(ansi --yellow --italic "$2")"
		if [[ "$COLORS" == 1 ]]; then
			echo -e "[$pfx]   $msg"
		else
			echo -e "[$1]   $2"
		fi
	}

	while read -r l; do
		ok "TEST" "$l"
	done < <(eval "$cmd")

}

test_builtin color color "$DEFAULT_POST_CMD"
