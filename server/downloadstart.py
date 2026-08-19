#!/usr/bin/env python3
# Fetch latest mustdice_server.py from GitHub and exec it.
import os
import sys
import urllib.request

GITHUB_RAW_URL = (
    "https://raw.githubusercontent.com/realryo1/MustDice/master/server/mustdice_server.py"
)

HERE = os.path.dirname(os.path.abspath(__file__))
TARGET = os.path.join(HERE, "mustdice_server.py")


def download() -> bool:
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


def main() -> None:
    ok = download()
    if not ok and not os.path.isfile(TARGET):
        sys.exit(1)
    os.chdir(HERE)
    os.execv(sys.executable, [sys.executable, TARGET])


if __name__ == "__main__":
    main()
