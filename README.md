# Mini Linux Shell with Process Manager

A custom Linux shell written in C++17 that builds its own pipeline of POSIX
calls instead of delegating to bash: `fork()`, `execvp()`, `waitpid()`,
`pipe()`, `dup2()`, `sigaction()`, `setpgid()`/`tcsetpgrp()`, and the Linux
`/proc` filesystem.

It is a usability subset of bash, with the parts that matter for an OS course
implemented from scratch: process creation, job control, redirection, pipelines,
signal handling and process monitoring.

## Features

| Area | What works |
|---|---|
| Execution | external programs via `execvp`, multiple arguments, quoted arguments |
| Builtins | `cd`, `pwd`, `exit`, `jobs`, `history`, `kill`, `fg`, `bg`, `ps`, `pinfo`, `monitor`, `help` |
| Pipelines | `a | b | c` (any number of stages), all stages waited for |
| Redirection | `< in`, `> out`, `>> out` |
| Background | `cmd &`, job ids, `jobs`, completion notices at the prompt |
| History | `history`, `!N` recall of entry N |
| Job control | own process groups, terminal handoff (`tcsetpgrp`), Ctrl+C / Ctrl+Z, `fg` / `bg` |
| Signals | `SIGCHLD` via self-pipe, shell survives Ctrl+C, children reaped |
| Monitoring | `monitor`, `monitor --pid N`, `ps`, `pinfo N` reading `/proc/[pid]/stat`, `cmdline`, `meminfo`, `uptime` |

## Build

Linux or WSL2 (Ubuntu 22.04/24.04). From the project root:

```bash
cmake -S . -B build
cmake --build build -j
./build/minishell
```

## Usage

```text
$ pwd
/mnt/d/OS project
$ echo "Hello World"
Hello World
$ ls | grep CMake
CMakeLists.txt
$ ls > files.txt
$ cat < files.txt
$ echo another >> files.txt
$ sleep 30 &
[1] Started  sleep 30 &  PID: 4210
$ jobs
[1] Running  sleep 30 &  PID: 4210
$ kill %1
$ monitor --pid 4210
$ pinfo 1
```

Ctrl+C interrupts the foreground job without killing the shell. Ctrl+Z stops
it; `jobs` shows it as `Stopped`; `bg 1` resumes it in the background and
`fg 1` brings it back to the terminal.

Type `help` inside the shell for the builtin list, and `exit` to leave.

## Tests

```bash
./build/parser_tests        # parser: quoting, pipes, redirection, errors
./build/monitor_tests       # /proc parser: fixtures + the live process
python3 tests/pty_smoke.py  # Ctrl+C, Ctrl+Z, jobs, bg, fg through a pty
```

The behavioral checklist and recorded results live in
[docs/test_cases.md](docs/test_cases.md).

## Layout

```text
src/
  main.cpp
  shell/      prompt + select-driven line reader
  parser/     pure tokenizer/parser -> ParsedLine
  executor/   builtins + fork/exec pipeline runner
  process/    job table, terminal job control
  ipc/        input/output/append redirection
  signals/    self-pipe SIGCHLD/SIGINT handler
  monitor/    /proc reader and report commands
  history/    in-memory command history
tests/        parser tests, monitor tests, pty smoke test
docs/         architecture, system calls, test cases
```

## Limitations

- No line editing library: no arrow keys, tab completion, or persistent history.
- No environment variable expansion, globbing, `&&`, `||`, `;`, or subshells.
- Background job output can interleave with the prompt (no redraw).
- `monitor` lists cumulative CPU time; `pinfo` adds a 200 ms two-sample CPU
  percentage.
- Job control requires a real terminal. With piped stdin the shell falls back to
  shared process groups and terminal signals behave like a non-interactive
  shell.
- Linux/WSL only, by design.

## Documentation

- [Architecture](docs/architecture.md)
- [System calls used](docs/system_calls.md)
- [Test cases and results](docs/test_cases.md)
- [Development rules](RULES.md)
- [Work log](LOGS.md)
