#!/usr/bin/env python3
"""Capture real minishell sessions through a pty, for screenshots.

Run inside WSL from the repository root:
    python3 submission/pptx-build/capture_sessions.py
Writes submission/pptx-build/transcripts.json.
"""
import fcntl
import json
import os
import pty
import re
import select
import shutil
import struct
import subprocess
import termios
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BIN = os.path.join(ROOT, "build", "minishell")
HOME = os.path.expanduser("~")
DEMO = os.path.join(HOME, "minishell-demo")
LINK = os.path.join(HOME, "mini-linux-shell")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "transcripts.json")

ANSI = re.compile(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07|\x1b[()][A-Z0-9]")


def clean(data):
    text = data.decode("utf-8", "replace").replace("\r\n", "\n").replace("\r", "")
    text = ANSI.sub("", text)
    return text


def setup():
    if os.path.exists(DEMO):
        shutil.rmtree(DEMO)
    os.makedirs(DEMO)
    shutil.copy2(BIN, os.path.join(DEMO, "minishell"))
    os.chmod(os.path.join(DEMO, "minishell"), 0o755)
    files = {
        "notes.txt": "kernel notes\n",
        "report.txt": "experiment results\n",
        "todo.txt": "write report\n",
        "build.log": "build finished\n",
        "README": "minishell demo files\n",
    }
    for name, body in files.items():
        with open(os.path.join(DEMO, name), "w", encoding="utf-8") as f:
            f.write(body)
    if os.path.islink(LINK):
        os.remove(LINK)
    elif os.path.isdir(LINK):
        shutil.rmtree(LINK)
    os.makedirs(LINK)
    os.symlink(os.path.join(ROOT, "build"), os.path.join(LINK, "build"))
    os.symlink(os.path.join(ROOT, "tests"), os.path.join(LINK, "tests"))


def child_setup(slave):
    os.setsid()
    fcntl.ioctl(slave, termios.TIOCSCTTY, 0)


class Session:
    def __init__(self, cwd, binary=None):
        self.master, slave = pty.openpty()
        fcntl.ioctl(self.master, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 120, 0, 0))
        self.proc = subprocess.Popen(
            [binary or os.path.join(cwd, "minishell")],
            cwd=cwd,
            stdin=slave,
            stdout=slave,
            stderr=slave,
            preexec_fn=lambda: child_setup(slave),
            close_fds=True,
        )
        os.close(slave)
        self.raw = b""

    def read(self, timeout=0.1):
        r, _, _ = select.select([self.master], [], [], timeout)
        if not r:
            return b""
        try:
            data = os.read(self.master, 65536)
        except OSError:
            return b""
        if data:
            self.raw += data
        return data

    def wait_prompt(self, timeout=20):
        end = time.time() + timeout
        while time.time() < end:
            self.read(0.1)
            if clean(self.raw).endswith("$ "):
                self.read(0.05)
                if clean(self.raw).endswith("$ "):
                    return clean(self.raw)
        raise TimeoutError(clean(self.raw[-2000:]))

    def send(self, text):
        os.write(self.master, text.encode())

    def settle(self, timeout=20, quiet=0.4):
        end = time.time() + timeout
        last = time.time()
        while time.time() < end:
            data = self.read(0.05)
            if data:
                last = time.time()
            elif time.time() - last > quiet:
                break
        return clean(self.raw)

    def command(self, text, timeout=20):
        self.send(text + "\n")
        return self.settle(timeout)

    def command_prompt(self, text, timeout=120):
        self.send(text + "\n")
        return self.wait_prompt(timeout)

    def close(self):
        try:
            self.proc.terminate()
            self.proc.wait(timeout=2)
        except Exception:
            self.proc.kill()
        try:
            os.close(self.master)
        except OSError:
            pass


def capture_quick():
    s = Session(DEMO)
    s.wait_prompt()
    s.command("ls")
    s.command("ls | grep txt | wc -l")
    s.command('echo "Hello World"')
    s.command("echo hi > out.txt")
    s.command("cat < out.txt")
    s.command("echo second line >> out.txt")
    s.command("cat < out.txt")
    s.command("history")
    text = clean(s.raw)
    s.close()
    return text


def capture_jobs():
    s = Session(DEMO)
    s.wait_prompt()
    s.send("sleep 30\n")
    time.sleep(0.8)
    s.send("\x1a")
    s.settle()
    s.command("jobs")
    s.command("bg 1")
    s.command("jobs")
    s.send("fg 1\n")
    time.sleep(0.8)
    s.send("\x03")
    s.settle()
    s.command("echo survived")
    s.command("sleep 60 &")
    s.command("jobs")
    s.command("kill %2")
    text = clean(s.raw)
    s.close()
    return text


def capture_monitor():
    s = Session(DEMO)
    s.wait_prompt()
    s.command("yes > /dev/null &")
    out = clean(s.raw)
    m = re.search(r"PID:\s*(\d+)", out)
    pid = m.group(1) if m else "1"
    s.command("ps")
    s.command("pinfo " + pid)
    s.command("kill %1")
    text = clean(s.raw)
    s.close()
    return text


def capture_tests():
    s = Session(LINK, os.path.join(LINK, "build", "minishell"))
    s.wait_prompt()
    s.command_prompt("ctest --test-dir build --output-on-failure | tail -9", timeout=60)
    s.command_prompt("python3 tests/pty_smoke.py | tail -2", timeout=120)
    s.send("exit\n")
    time.sleep(0.5)
    text = clean(s.raw)
    s.close()
    return text


def main():
    setup()
    sessions = [
        {"name": "quick_start", "title": "minishell  |  quick start", "text": capture_quick()},
        {"name": "job_control", "title": "minishell  |  jobs and signals", "text": capture_jobs()},
        {"name": "monitor", "title": "minishell  |  process monitor", "text": capture_monitor()},
        {"name": "tests", "title": "minishell  |  test run", "text": capture_tests()},
    ]
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(sessions, f, indent=2)
    print("wrote", OUT)
    for s in sessions:
        print("---", s["name"], "---")
        print(s["text"][-400:])


if __name__ == "__main__":
    main()
