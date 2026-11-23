# Viva Questions and Answers

Answers reference this codebase; file names point at where the mechanism lives.

## Process creation and execution

**Q1. What happens between typing `ls -l` and seeing output?**
The shell reads the line, `parse_line()` builds a `ParsedLine` (program, args,
flags). `cd`, `pwd`, `exit`, `jobs`, `history`, `kill`, `fg`, `bg`, `ps`,
`pinfo`, `monitor`, `help` run in-process. Anything else goes through
`run_external()`: `fork()` creates a child, the child resets signal
dispositions, wires fds, `execvp()` replaces its image with `/bin/ls`, the
parent `waitpid()`s and takes the terminal back.

**Q2. Why `execvp()` and not `execve()`?**
`execvp` searches `PATH` for us and passes the current `environ`. With `execve`
we would have to split `PATH` and try each candidate ourselves — work that adds
nothing to the OS concepts being demonstrated.

**Q3. Why does `cd` have to be a builtin?**
`chdir()` changes the working directory of *the calling process*. In a child,
that change disappears with the child. The shell must call `chdir()` itself.

**Q4. What does `fork()` return and what does the child inherit?**
`0` in the child, the child's pid in the parent, `-1` on failure. The child
inherits file descriptors, signal dispositions, working directory and memory
(copy-on-write). That inheritance is why the child closes pipe ends the parent
opened and resets handlers before `exec`.

**Q5. Why not `system()` or `popen()`?**
Both spawn `/bin/sh` and hide process creation, waiting, pipes and signal
handling behind one call. This project implements those mechanisms, so using
them would defeat the purpose.

## Waiting, zombies and reaping

**Q6. What is a zombie and where could one appear here?**
A child that has exited but whose exit status the parent has not collected.
Before `waitpid()`, the entry stays in the process table. Any finished child in
`job_manager.cpp` is collected by `waitpid(-1, WNOHANG | WUNTRACED |
WCONTINUED)`; foreground children are collected by the executor's blocking
`waitpid(pid, ..., WUNTRACED)`.

**Q7. Why `WNOHANG` in the reaper but not in the foreground wait?**
Background reaping must never block the prompt, so it is a non-blocking drain.
The foreground wait is *supposed* to block until that specific child exits or
stops.

**Q8. What is `WUNTRACED` for?**
It also reports children that stopped (Ctrl+Z) instead of only exited ones. That
is what turns a stopped foreground job into a `Stopped` entry in the job table.

**Q9. Why `EINTR` loops around `waitpid` and `select`?**
Signals can interrupt blocking calls. After `SIGCHLD` or Ctrl+C, `waitpid` may
return `-1` with `errno == EINTR`; retrying is correct because the child was not
reaped by that failed call.

**Q10. Why isn't `waitpid()` called inside the signal handler?**
Handlers run asynchronously, possibly interrupting `malloc` or iostream state.
`waitpid` itself is async-signal-safe, but updating the job table
(`std::vector`) inside a handler is undefined behavior. So the handler only
writes a byte to a self-pipe; the main loop does the real work.

**Q11. How does the shell learn about a finished background job while sitting
at the prompt?**
`SIGCHLD` handler → byte in the self-pipe. `Shell::read_line()` uses `select()`
on stdin *and* the pipe, wakes on the pipe, reaps, and prints `[n] Done ...`.

## Signals and job control

**Q12. Why doesn't Ctrl+C kill the shell?**
The shell ignores `SIGINT` (and `SIGQUIT`, `SIGTSTP`). With job control the
terminal belongs to the foreground job's process group, so the kernel delivers
the signal to that group, not to the shell.

**Q13. What is a process group and why create one per job?**
A process group is what the terminal sends signals to (`tcsetpgrp` sets the
foreground pgrp). Giving each job its own group means Ctrl+C/Ctrl+Z hit exactly
that job, and a background job cannot receive the terminal's signals.

**Q14. What is an orphaned process group and why did it matter here?**
A group is orphaned when no member has a parent in the same session outside the
group. POSIX discards `SIGTSTP` for orphaned groups. The first implementation
put foreground children in the shell's group; with the shell as session leader
the group was orphaned, so Ctrl+Z silently did nothing. Fixed by proper
`setpgid` + `tcsetpgrp` job control.

**Q15. Why ignore SIGTTOU and SIGTTIN in the shell?**
`tcsetpgrp()` from a background process raises `SIGTTOU` by default. The shell
must ignore it to take the terminal back after a job finishes.

**Q16. How does `fg` work?**
Send `SIGCONT` to the job's pids, `tcsetpgrp()` the job's pgrp, `waitpid` until
it exits or stops again. `bg` only sends `SIGCONT` and leaves the terminal with
the shell.

**Q17. What happens to background jobs when the shell exits?**
`terminate_all()` sends `SIGHUP` to every tracked pid, so we do not orphan work
behind us.

## Pipes and file descriptors

**Q18. How does `ls | grep txt` avoid deadlock?**
The parent creates `pipe()`, forks stage 1 with stdout `dup2`-ed to the write
end, forks stage 2 with stdin from the read end, then closes both ends in the
parent and waits. Closing the unused ends everywhere is what allows EOF to
propagate: once the writer has no open write end, the reader sees EOF.

**Q19. What would happen if the parent forgot to close a pipe end?**
The reader would never see EOF because *some* process still holds the write end
open. `grep` would hang after `ls` exits. This is the classic fd-leak bug.

**Q20. Why `dup2()` instead of assigning fd numbers?**
`dup2(old, new)` makes `new` a copy of `old` atomically, closing whatever `new`
previously was. The child's `exec` output then goes to the pipe/file because fd
1 was replaced.

**Q21. What's the difference between `>` and `>>`?**
`O_TRUNC` vs `O_APPEND` in `open()`. Both use `O_WRONLY | O_CREAT` with mode
`0644`.

## Monitoring and /proc

**Q22. How is `/proc/[pid]/stat` parsed safely?**
Fields are space-separated, but field 2 (`comm`) may contain spaces and
parentheses. Parsing starts after the *last* `)` instead of splitting on
whitespace. Verified with a fixture containing `(my prog)`.

**Q23. What's the difference between `utime`/`stime` and CPU utilization?**
`utime`/`stime` are cumulative clock ticks the process has used. Utilization is
a rate: (delta ticks / `CLK_TCK`) / interval. `pinfo` samples the stat file
twice, 200 ms apart, to report the momentary percentage.

**Q24. Why is `rss` multiplied by the page size?**
`/proc/[pid]/stat` reports `rss` in pages. `sysconf(_SC_PAGESIZE)` converts to
bytes; `human_size()` renders it.

**Q25. What can prevent reading another process's `/proc` entry?**
Permissions and kernel hardening (`hidepid`, Yama). The monitor reports what it
can read; an unreadable pid is skipped or reported as missing.

## Design and limitations

**Q26. Is this an operating system?**
No. It is a user-space program using kernel services through POSIX calls. The
kernel still does scheduling, memory management, and real process creation.

**Q27. Why is history in memory only?**
Persistence was not required, and a file format plus `$HOME` handling adds
surface without demonstrating OS concepts. Listed as a limitation.

**Q28. Why no readline?**
Line editing is terminal UI, not process management. `select()` already gives
us the one behavior that matters: waking for signals at the prompt.

**Q29. What is the biggest remaining correctness gap?**
Interactive line editing and redrawing the prompt around job notices; output
from background jobs can interleave with typed input. Also no globbing or
environment expansion, so the shell is a deliberate subset.
