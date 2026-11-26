# Architecture

## Process model

```text
                 user
                  |
                  v
        +--------------------+
        |  Shell loop        |  prompt, read_line (select)
        +--------------------+
             |          ^
     parse   |          | SIGCHLD byte
             v          | (self-pipe)
        +----------+   +-----------------+
        |  Parser  |   | Signal handler  |  write() only
        +----------+   +-----------------+
             |
             v
        +--------------------+
        |     Executor       |  builtins vs external
        +--------------------+
          |        |       |
          v        v       v
     JobManager   ipc    terminal
     (jobs,reap) (dup2)  (pgid/tcsetpgrp)
          |        |       |
          +--------+-------+
                   |
                   v
             fork / execvp
                   |
                   v
             Linux kernel
                   |
                   v
              /proc filesystem
                   |
                   v
          Procmon: monitor, ps, pinfo
```

## Components

| Component | Files | Responsibility |
|---|---|---|
| Shell loop | `src/shell/shell.cpp` | prompt, read a line, feed parser/executor, print job notices |
| Parser | `src/parser/parser.cpp` | pure tokenizer: quotes, `|`, `<`, `>`, `>>`, `&` -> `ParsedLine` |
| Executor | `src/executor/executor.cpp` | dispatch builtins, fork/exec pipelines, foreground wait, bg jobs |
| Job table | `src/process/job_manager.cpp` | job ids, pids, states, reaping, `jobs`, `SIGHUP` at exit |
| Terminal control | `src/process/terminal.cpp` | shell pgid, `tcsetpgrp` handoff, no-op without tty |
| IPC | `src/ipc/ipc.cpp` | `open`/`dup2`/`close` for `<`, `>`, `>>` |
| Signals | `src/signals/signal_handler.cpp` | `sigaction` handlers, self-pipe, child reset |
| Monitor | `src/monitor/process_monitor.cpp` | `/proc` parsing and report commands |
| History | `src/history/history.cpp` | in-memory command list |

## Data model

```cpp
struct Command {
    std::string program;
    std::vector<std::string> args;
    std::string infile, outfile;
    bool append;
};

struct ParsedLine {
    std::vector<Command> stages;  // pipeline stages
    bool background;
    bool ok;
    std::string error;
};

struct Job {
    int id;
    std::vector<pid_t> pids;      // every stage of the job
    std::string command;
    JobState state;               // Running, Stopped, Done
};
```

`ParsedLine` is the only contract between parser and executor. The parser never
touches the OS; the executor never re-parses strings.

## Execution flows

### Foreground command

1. `run_builtin` handles the twelve builtins in-process (`cd` must change the
   shell's own cwd). Builtins are only considered for a single-stage line,
   because a pipeline stage would run in a child.
2. Otherwise `fork()`; the child resets signal dispositions, creates a new
   process group (when job control is available), wires pipe/redirection fds,
   then `execvp()`.
3. The parent closes unused pipe ends, gives the terminal to the child's pgid,
   and `waitpid(pid, WUNTRACED)`.
4. When the child exits or stops, the parent takes the terminal back. A stopped
   child becomes a `Stopped` job; for a pipeline, every stage pid is recorded
   but only one `[n] Stopped` line is printed.

### Pipeline

One `pipe()` per gap. Stage *i* reads from the previous pipe read end and writes
to the next pipe write end. Both ends the stage does not use are closed before
`execvp()`, so EOF propagates correctly. The foreground waiter waits for every
stage; if one stops, the group is stopped as a unit, so it records all stage
pids and returns after the first stop.

### Background job

Same fork path, but the parent does not wait. The job (all stage pids) is added
to the job table with a new id, and the prompt returns immediately. At the next
main-loop iteration, `waitpid(-1, WNOHANG | WUNTRACED | WCONTINUED)` collects
finished children and prints `[n] Done ...` notices.

### Reaping and the self-pipe

The `SIGCHLD` handler writes one byte to a pipe. The shell's `select()` waits on
stdin *and* that pipe, so completion notices appear while the user is idle at
the prompt without doing unsafe work in the handler. All `waitpid()` calls and
container updates happen in normal code. `SIGINT` uses the same pipe: when the
line reader drains a SIGINT byte it drops any partial input and redraws the
prompt.

### Job control

With a tty, the shell puts itself in its own process group and hands terminal
ownership (`tcsetpgrp`) to the foreground job's process group. Ctrl+C and
Ctrl+Z therefore reach the job, not the shell. `fg` re-grants the terminal and
sends `SIGCONT`; `bg` only sends `SIGCONT`, leaving the job in the background.
Ctrl+C at the prompt reaches the shell through the SIGINT self-pipe instead,
which is why the shell stays alive and redraws the prompt. Without a tty, the
shell skips all of this and children share its group — the behavior of a
non-interactive shell.

### Monitoring

`/proc/[pid]/stat` is parsed starting after the last `)` so process names with
spaces and parentheses are safe. `utime`/`stime` are cumulative clock ticks
(field 14/15); `vsize` and `rss` come from fields 23/24. `cmdline` is read as
NUL-separated bytes and rendered with spaces. `/proc/meminfo` and `/proc/uptime`
provide the header summary. CPU utilization is a rate, so `pinfo` and
`monitor --pid` take a second snapshot 200 ms later and report the tick delta as
a percentage; the table view stays cumulative.

## Error handling

- `fork`, `pipe`, `open`, `dup2`, `execvp`, `kill`, `chdir`, `tcsetpgrp` all
  report through `perror`/formatted messages; the shell keeps running.
- A failed `execvp` exits the child with status 127 after printing the reason.
- `waitpid` retries on `EINTR`; `select` re-enters on `EINTR`.
- On exit, background jobs receive `SIGHUP` so the shell does not leave work
  running behind it.
