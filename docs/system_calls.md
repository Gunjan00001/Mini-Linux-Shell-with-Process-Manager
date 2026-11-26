# System Calls and Kernel Interfaces

Every OS operation in this project goes through a POSIX call. This is the map
from feature to interface.

| Call / file | Where | Why |
|---|---|---|
| `fork()` | `executor/executor.cpp` | create the child that will run the program |
| `execvp()` | `executor/executor.cpp` | replace the child image, search `PATH` |
| `waitpid()` | `executor`, `job_manager` | synchronize with children, collect exit/stop status, reap zombies |
| `pipe()` | `executor` | connect stdout of stage *i* to stdin of stage *i+1* |
| `dup2()` | `ipc`, `executor` | move pipe ends and files onto fd 0/1 (stdin/stdout) |
| `open()` | `ipc` | open `<`, `>`, `>>` targets (`O_RDONLY`, `O_TRUNC`, `O_APPEND`) |
| `close()` | `ipc`, `executor`, `signals` | release unused pipe ends and files in parent and child |
| `kill()` | `executor`, `job_manager` | `kill` builtin, `fg`/`bg` (`SIGCONT`), exit cleanup (`SIGHUP`) |
| `killpg()` | `executor`, `job_manager` | signal every stage of a job through its process group |
| `sigaction()` | `signals` | install `SIGCHLD`/`SIGINT` handlers and restore `SIG_DFL` in children |
| `signal()` | `signals` | ignore `SIGQUIT`, `SIGTSTP`, `SIGTTIN`, `SIGTTOU` in the shell |
| `setpgid()` | `executor`, `terminal` | put jobs in their own process group |
| `tcsetpgrp()` | `terminal` | hand the controlling terminal to the foreground job and take it back |
| `tcgetpgrp()` | `terminal` | verify the shell actually owns the terminal |
| `isatty()` | `terminal` | job control only when stdin is a terminal |
| `select()` | `shell` | wait on stdin *and* the signal self-pipe simultaneously |
| `read()` | `shell`, `signals` | line reader and self-pipe drain |
| `write()` | `signals` | async-signal-safe notification from the handler |
| `chdir()` | `executor` | `cd` builtin (must run in the shell process) |
| `getcwd()` | `executor`, `shell` | `pwd` builtin and the prompt |
| `getpid()` | `executor`, `monitor` | shell pid, `ps` parent filter |
| `_exit()` | `executor` | child exit on failed redirection or `execvp` |
| `fcntl()` | `signals` | non-blocking + close-on-exec self-pipe ends |
| `sysconf(_SC_CLK_TCK)` | `monitor` | convert `utime`/`stime` ticks to seconds |
| `sysconf(_SC_PAGESIZE)` | `monitor` | convert `rss` pages to bytes |

## /proc interfaces

| Path | Fields used | Feature |
|---|---|---|
| `/proc/[pid]/stat` | state (3), ppid (4), utime (14), stime (15), num_threads (20), vsize (23), rss (24) | `monitor`, `ps`, `pinfo` |
| `/proc/[pid]/cmdline` | NUL-separated argv | `pinfo`, `monitor --pid` |
| `/proc/meminfo` | `MemTotal`, `MemAvailable` | monitor header |
| `/proc/uptime` | first field | monitor header |

The `stat` parser starts from the last `)` instead of splitting on spaces,
because the `comm` field is allowed to contain spaces and parentheses.

## Signals handled

| Signal | Shell | Child |
|---|---|---|
| `SIGCHLD` | handler writes to self-pipe; main loop reaps | reset to default |
| `SIGINT` | handler writes to self-pipe; prompt loop drops partial input and redraws | default: terminate |
| `SIGQUIT` | ignored | default |
| `SIGTSTP` | ignored (terminal goes to the job) | default: stop |
| `SIGTTIN`/`SIGTTOU` | ignored (needed for `tcsetpgrp`) | default |
| `SIGCONT` | default | default |
| `SIGHUP` | sent to jobs on shell exit | default: terminate |
