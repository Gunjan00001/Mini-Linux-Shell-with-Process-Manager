# Project Rules

Rules we hold ourselves to while building this shell. Change them only with a
reason recorded in `LOGS.md`.

## 1. Scope

- Linux only (WSL2/Ubuntu 24.04 in practice). Everything runs in `wsl`.
- Real kernel mechanisms only: `fork`, `execvp`, `waitpid`, `pipe`, `dup2`,
  `open`, `close`, `kill`, `sigaction`, `setpgid`, `tcsetpgrp`, `/proc`.
- Never use `system()`, `popen()`, or shell out to bash to get behavior.
- The project is a user-space shell. No kernel, scheduler, or memory manager.

## 2. Process and signal rules

- A signal handler does exactly one thing: `write()` a byte to the self-pipe.
  No allocation, no containers, no iostream, no `errno` fiddling.
- `waitpid()` is only called from the main loop or the foreground waiter.
  Never from a handler.
- Shell ignores `SIGINT`/`SIGQUIT`/`SIGTSTP`/`SIGTTIN`/`SIGTTOU`; children get
  `SIG_DFL` restored right after `fork()`.
- Every child that is created is either reaped by `waitpid()` or tracked in the
  job table. No zombies, no orphans left at exit (`SIGHUP` on shell exit).
- Background jobs get their own process group. Foreground jobs get their own
  group too when a tty is available, and own the terminal via `tcsetpgrp()`.

## 3. File descriptor rules

- Parent closes every pipe end it does not use, immediately after `fork()`.
- Child closes unused ends before `dup2()`-ing what it needs.
- Redirection paths close the opened fd after `dup2()`.
- Any new code path that opens a descriptor must have a matching `close()` on
  every exit path, including error paths.

## 4. Code rules

- C++17, `-Wall -Wextra -Wpedantic` clean.
- Types `PascalCase`, functions/variables `snake_case`, members trailing `_`.
- Parser and `/proc` parsing stay pure functions (string in, struct out) so
  they can be unit tested without a terminal or processes.
- No new dependency for something the standard library or POSIX already does.
- Deliberate simplifications get a comment naming the ceiling.

## 5. Testing rules

- Parser and `/proc` parsing: assert tests (`tests/parser_tests.cpp`,
  `tests/monitor_tests.cpp`). Tests must still assert in Release builds.
- Interactive behavior: `tests/pty_smoke.py` (Ctrl+C, Ctrl+Z, jobs, bg, fg).
- Behavioral checklist in `docs/test_cases.md` before claiming a milestone.
- After pipe/redirect changes, check fd count in `/proc/<shellpid>/fd` and
  confirm no zombies with `ps --ppid <shellpid>`.

## 6. Git rules

- One logical change per commit; small commits are preferred over one big one.
- Conventional prefixes: `feat(scope):`, `fix(scope):`, `test:`, `docs:`,
  `chore:`.
- Tags are semver: `vMAJOR.MINOR.PATCH` (`v0.1.0`, `v1.0.0`).
- `LOGS.md` is updated after the work it describes, with commit hashes.
- Build must pass before pushing. `python3 tests/pty_smoke.py` must pass before
  tagging a release.

## 7. Build and run

```bash
# inside WSL, from the project root
cmake -S . -B build
cmake --build build -j
./build/minishell
```

Tests:

```bash
./build/parser_tests && ./build/monitor_tests
python3 tests/pty_smoke.py
```
