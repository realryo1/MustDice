#!/usr/bin/env python3
# Admin HTTP: GitHub fetch / upload / start game tmux / logs.
# Hand-placed with startadmin.sh. Game start is only from this browser.
import base64
import cgi
import json
import os
import shutil
import subprocess
import sys
import urllib.request

try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
except ImportError:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer

HERE = os.path.dirname(os.path.abspath(__file__))
TARGET = os.path.join(HERE, "mustdice_server.py")
TMUX_SESSION = "mustdice"
DEFAULT_PORT = 8777
UPLOAD_MAX = 2 * 1024 * 1024
MARKER = b"def score_pinpoint"
GITHUB_RAW_URL = (
    "https://raw.githubusercontent.com/realryo1/MustDice/master/server/mustdice_server.py"
)

USER = os.environ.get("MUSTDICE_ADMIN_USER", "")
PASS = os.environ.get("MUSTDICE_ADMIN_PASS", "")
PORT = int(os.environ.get("MUSTDICE_ADMIN_PORT", str(DEFAULT_PORT)))


def secrets_equal(a, b):
    if len(a) != len(b):
        return False
    result = 0
    for x, y in zip(a.encode("utf-8"), b.encode("utf-8")):
        result |= x ^ y
    return result == 0


def tmux_running():
    return (
        subprocess.call(
            ["tmux", "has-session", "-t", TMUX_SESSION],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        == 0
    )


def capture_logs():
    if not tmux_running():
        return "(tmux session mustdice is not running)\n"
    try:
        out = subprocess.check_output(
            ["tmux", "capture-pane", "-t", TMUX_SESSION, "-S", "-2000", "-p"],
            stderr=subprocess.STDOUT,
        )
        return out.decode("utf-8", errors="replace")
    except Exception as e:
        return "capture failed: %s\n" % e


def save_server_bytes(data):
    if not data or len(data) > UPLOAD_MAX:
        return False, "file empty or too large"
    if MARKER not in data:
        return False, "not mustdice_server.py (missing def score_pinpoint)"
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return False, "not utf-8"
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    tmp = TARGET + ".upload.tmp"
    with open(tmp, "wb") as f:
        f.write(text.encode("utf-8"))
    os.replace(tmp, TARGET)
    return True, "wrote %s (%s bytes)\n" % (TARGET, len(text.encode("utf-8")))


def fetch_from_github():
    try:
        with urllib.request.urlopen(GITHUB_RAW_URL, timeout=20) as resp:
            data = resp.read()
    except Exception as e:
        return False, "fetch failed: %s\n" % e
    return save_server_bytes(data)


def start_game_detached():
    if shutil.which("tmux") is None:
        return False, "tmux not found\n"
    if not os.path.isfile(TARGET):
        return False, "missing %s\n" % TARGET
    if tmux_running():
        subprocess.call(["tmux", "kill-session", "-t", TMUX_SESSION])
    rc = subprocess.call(
        [
            "tmux",
            "new-session",
            "-d",
            "-s",
            TMUX_SESSION,
            "-c",
            HERE,
            sys.executable,
            TARGET,
        ]
    )
    if rc != 0:
        return False, "tmux new-session failed\n"
    return True, "started detached session %s\n" % TMUX_SESSION


PAGE = """<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MustDice 管理</title>
<style>
body { font-family: sans-serif; margin: 24px; background: #111; color: #eee; }
button { font-size: 16px; padding: 8px 16px; margin-right: 8px; }
pre { background: #000; color: #9f9; padding: 12px; overflow: auto; max-height: 70vh; white-space: pre-wrap; }
.status { margin: 12px 0; }
.msg { margin: 12px 0; color: #fc6; }
</style>
</head>
<body>
<h1>MustDice 管理</h1>
<p class="status">ゲーム tmux (<code>mustdice</code>): <strong id="st">...</strong></p>
<p>
<button type="button" id="fetch">GitHubの最新を取得</button>
<button type="button" id="restart">起動 / 再起動</button>
<input type="file" id="file" accept=".py">
<button type="button" id="upload">アップロード</button>
</p>
<p class="msg" id="msg"></p>
<pre id="log"></pre>
<script>
function load() {
  fetch("/api/status", {credentials: "same-origin"}).then(function(r) {
    return r.json();
  }).then(function(d) {
    document.getElementById("st").textContent = d.running ? "稼働中" : "停止";
    document.getElementById("log").textContent = d.logs || "";
  }).catch(function() {
    document.getElementById("st").textContent = "取得失敗";
  });
}
function post(path) {
  document.getElementById("msg").textContent = "実行中...";
  fetch(path, {method: "POST", credentials: "same-origin"}).then(function(r) {
    return r.json();
  }).then(function(d) {
    document.getElementById("msg").textContent = (d.ok ? "OK " : "失敗 ") + (d.output || "");
    load();
  }).catch(function(e) {
    document.getElementById("msg").textContent = String(e);
  });
}
document.getElementById("fetch").onclick = function() { post("/fetch"); };
document.getElementById("restart").onclick = function() { post("/restart"); };
document.getElementById("upload").onclick = function() {
  var f = document.getElementById("file").files[0];
  if (!f) {
    document.getElementById("msg").textContent = "ファイルを選んでください";
    return;
  }
  if (f.name !== "mustdice_server.py") {
    document.getElementById("msg").textContent = "mustdice_server.py以外をアップロードしようとしています";
    return;
  }
  document.getElementById("msg").textContent = "実行中...";
  var fd = new FormData();
  fd.append("file", f, f.name);
  fetch("/upload", {method: "POST", credentials: "same-origin", body: fd}).then(function(r) {
    return r.json();
  }).then(function(d) {
    document.getElementById("msg").textContent = (d.ok ? "OK " : "失敗 ") + (d.output || "");
    load();
  }).catch(function(e) {
    document.getElementById("msg").textContent = String(e);
  });
};
load();
setInterval(load, 3000);
</script>
</body>
</html>
"""


class Handler(BaseHTTPRequestHandler):
    def _unauthorized(self):
        self.send_response(401)
        self.send_header("WWW-Authenticate", 'Basic realm="MustDice admin"')
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.end_headers()
        self.wfile.write(b"auth required\n")

    def _check_auth(self):
        header = self.headers.get("Authorization") or ""
        if not header.startswith("Basic "):
            return False
        try:
            raw = base64.b64decode(header.split(" ", 1)[1].strip())
            decoded = raw.decode("utf-8")
        except Exception:
            return False
        if ":" not in decoded:
            return False
        user, password = decoded.split(":", 1)
        return secrets_equal(user, USER) and secrets_equal(password, PASS)

    def _json(self, code, obj):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if not self._check_auth():
            self._unauthorized()
            return
        if self.path == "/" or self.path.startswith("/?"):
            body = PAGE.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.path == "/api/status":
            self._json(
                200,
                {"running": tmux_running(), "logs": capture_logs()},
            )
            return
        self.send_response(404)
        self.end_headers()

    def do_POST(self):
        if not self._check_auth():
            self._unauthorized()
            return
        if self.path == "/fetch":
            ok, out = fetch_from_github()
            self._json(200, {"ok": ok, "output": out})
            return
        if self.path == "/restart":
            ok, out = start_game_detached()
            self._json(200, {"ok": ok, "output": out})
            return
        if self.path == "/upload":
            try:
                form = cgi.FieldStorage(
                    fp=self.rfile,
                    headers=self.headers,
                    environ={
                        "REQUEST_METHOD": "POST",
                        "CONTENT_TYPE": self.headers.get("Content-Type", ""),
                    },
                )
            except Exception as e:
                self._json(200, {"ok": False, "output": str(e)})
                return
            item = form["file"] if "file" in form else None
            if item is None or not getattr(item, "file", None):
                self._json(200, {"ok": False, "output": "no file"})
                return
            name = os.path.basename((item.filename or "").replace("\\", "/"))
            if name != "mustdice_server.py":
                self._json(
                    200,
                    {"ok": False, "output": "filename must be mustdice_server.py"},
                )
                return
            data = item.file.read()
            ok, out = save_server_bytes(data)
            self._json(200, {"ok": ok, "output": out})
            return
        self.send_response(404)
        self.end_headers()

    def log_message(self, fmt, *args):
        sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))


def main():
    if not USER:
        sys.stderr.write("Set MUSTDICE_ADMIN_USER before start.\n")
        sys.stderr.flush()
        sys.exit(1)
    try:
        server = HTTPServer(("0.0.0.0", PORT), Handler)
    except OSError as e:
        sys.stderr.write("bind 0.0.0.0:%s failed: %s\n" % (PORT, e))
        sys.stderr.flush()
        sys.exit(1)
    sys.stderr.write("MustDice admin listening on 0.0.0.0:%s\n" % PORT)
    sys.stderr.flush()
    server.serve_forever()


if __name__ == "__main__":
    main()
