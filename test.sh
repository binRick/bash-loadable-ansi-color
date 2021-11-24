#!/usr/bin/env bash
set -e
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
COLORS=0
if ! command -v ansi; then
  alias ansi=$(pwd)/ansi
fi

ansi --cyan --bold "Epoch MS: $(date +%s%3N)"
ansi --magenta --bold "Epoch: $(date +%s)"


test_builtin() {
	local M="$1"
	local N="$2"
	local post_cmd="echo -e \"MYPID=\$MYPID\nTS=\$TS\nMS=\$MS\""
	local cmd="enable -f 'src/.libs/$M.so' $N && $N && $post_cmd"
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

test_builtin color color
