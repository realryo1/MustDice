#!/bin/sh
# MustDice admin (port 8777) in tmux session "mustdiceadmin".
# Password is empty. User defaults to admin.
# Do not run this from tmux session "mustdice".
# Existing tmux server does not inherit this shell's export, so pass env with env(1).
cd "$(dirname "$0")" || exit 1
export MUSTDICE_ADMIN_USER="${MUSTDICE_ADMIN_USER:-admin}"
export MUSTDICE_ADMIN_PASS="${MUSTDICE_ADMIN_PASS-}"
export MUSTDICE_ADMIN_PORT="${MUSTDICE_ADMIN_PORT:-8777}"

run_admin() {
	python3 mustdice_admin.py
	st=$?
	echo
	echo "startadmin: python exited $st"
	echo "startadmin: this shell stays so the message remains. exit to close."
	exec sh
}

if [ -n "$TMUX" ]; then
	cur="$(tmux display-message -p '#S' 2>/dev/null || true)"
	if [ "$cur" = "mustdice" ]; then
		echo "startadmin: do not start admin inside tmux session mustdice" >&2
		exit 1
	fi
	run_admin
fi

if ! command -v tmux >/dev/null 2>&1; then
	echo "startadmin: tmux not found, running in this shell" >&2
	run_admin
fi

if tmux has-session -t mustdiceadmin 2>/dev/null; then
	tmux kill-session -t mustdiceadmin
fi

exec tmux new-session -s mustdiceadmin -c "$(pwd)" \
	env \
	MUSTDICE_ADMIN_USER="$MUSTDICE_ADMIN_USER" \
	MUSTDICE_ADMIN_PASS="$MUSTDICE_ADMIN_PASS" \
	MUSTDICE_ADMIN_PORT="$MUSTDICE_ADMIN_PORT" \
	sh -c 'python3 mustdice_admin.py; st=$?; echo; echo startadmin: python exited $st; echo startadmin: this shell stays so the message remains. exit to close.; exec sh'
