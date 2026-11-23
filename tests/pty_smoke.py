#!/usr/bin/env python3
"""Manual pty smoke test: Ctrl+C, Ctrl+Z, jobs, bg, exit.

Needs a real terminal, so it drives the shell through a pseudo-terminal.
Run from the repo root after building:  python3 tests/pty_smoke.py
"""
import os
import pty
import select
import sys
import time

BIN = os.path.abspath("build/minishell")


def read(fd, seconds=0.8):
    out = b""
    end = time.time() + seconds
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                chunk = os.read(fd, 4096)
            except OSError:
                break
            if not chunk:
                break
            out += chunk
    return out.decode(errors="replace")


def main():
    if not os.path.exists(BIN):
        sys.exit("build first: cmake -S . -B build && cmake --build build")

    pid, fd = pty.fork()
    if pid == 0:
        os.execv(BIN, [BIN])

    transcript = []

    def step(send, wait=0.8, label=""):
        os.write(fd, send)
        text = read(fd, wait)
        transcript.append((label, text))
        return text

    step(b"sleep 30\n", 0.5, "start sleep")
    step(b"\x03", 1.0, "ctrl-c")
    step(b"echo survived\n", 0.8, "after ctrl-c")
    step(b"sleep 30\n", 0.5, "start sleep again")
    step(b"\x1a", 1.0, "ctrl-z")
    step(b"jobs\n", 0.8, "jobs")
    step(b"bg 1\n", 0.8, "bg 1")
    step(b"jobs\n", 0.8, "jobs after bg")
    step(b"fg 1\n", 0.8, "fg 1")
    step(b"\x03", 1.0, "ctrl-c fg job")
    step(b"echo fgtest\n", 0.8, "after fg ctrl-c")
    step(b"exit\n", 1.0, "exit")

    for label, text in transcript:
        print(f"--- {label} ---")
        print(text.replace("\x1b", "<ESC>"))

    joined = "".join(t for _, t in transcript)
    checks = {
        "shell survived ctrl-c": "survived" in joined,
        "ctrl-z created stopped job": "Stopped" in joined,
        "bg resumed job": "Running" in joined,
        "fg job interruptible": "fgtest" in joined,
    }
    failed = [name for name, ok in checks.items() if not ok]
    if failed:
        sys.exit("FAILED: " + ", ".join(failed))
    print("pty smoke test passed")


if __name__ == "__main__":
    main()
