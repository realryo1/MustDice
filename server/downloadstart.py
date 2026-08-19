#!/usr/bin/env python3
# Fetch latest mustdice_server.py and run it inside tmux session "mustdice".
import os
import shutil
import subprocess
import sys
import urllib.request

GITHUB_RAW_URL = (
    "https://raw.githubusercontent.com/realryo1/MustDice/master/server/mustdice_server.py"
)
TMUX_SESSION = "mustdice"
HERE = os.path.dirname(os.path.abspath(__file__))
TARGET = os.path.join(HERE, "mustdice_server.py")


def download():
    try:
        with urllib.request.urlopen(GITHUB_RAW_URL, timeout=20) as resp:
            data = resp.read()
        data = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        if not data or b"def score_pinpoint" not in data:
            print("downloadstart: fetched data looks invalid", file=sys.stderr)
            return False
        with open(TARGET, "wb") as f:
            f.write(data)
        print("downloadstart: saved", TARGET)
        return True
    except Exception as e:
        print("downloadstart: fetch failed:", e, file=sys.stderr)
        return False


def exec_server():
    os.chdir(HERE)
    os.execv(sys.executable, [sys.executable, TARGET])


def tmux_has_session():
    return (
        subprocess.call(
            ["tmux", "has-session", "-t", TMUX_SESSION],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        == 0
    )


def start_in_tmux():
    if tmux_has_session():
        subprocess.call(["tmux", "kill-session", "-t", TMUX_SESSION])
    os.chdir(HERE)
    os.execvp(
        "tmux",
        [
            "tmux",
            "new-session",
            "-s",
            TMUX_SESSION,
            "-c",
            HERE,
            sys.executable,
            TARGET,
        ],
    )


def main():
    ok = download()
    if not ok and not os.path.isfile(TARGET):
        sys.exit(1)
    if os.environ.get("TMUX"):
        exec_server()
    if shutil.which("tmux") is None:
        print("downloadstart: tmux not found, running in this shell", file=sys.stderr)
        exec_server()
    start_in_tmux()


if __name__ == "__main__":
    main()
