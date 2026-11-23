# Demo Script (5–8 minutes)

Record in a real terminal (not via pipes — job control needs a tty).
Terminal font large enough to read; clear the screen between sections.

## 1. Build (30 s)

```bash
cd ~/mini-linux-shell
cmake -S . -B build
cmake --build build -j
./build/minishell
```

Say: no bash is used for execution; everything below goes through `fork`,
`execvp`, `pipe`, `dup2`, `sigaction`.

## 2. Basic shell (45 s)

```text
pwd
ls
cd /tmp
pwd
echo "Hello World"
abcxyz
```

Point out: `cd` is a builtin because `chdir()` must affect the shell process;
`abcxyz` shows `execvp` failing inside the child while the shell survives.

## 3. Redirection and pipes (1 min)

```text
cd ~/mini-linux-shell
ls > files.txt
cat < files.txt
echo another line >> files.txt
cat < files.txt
ls | grep CMake
ls | wc -l
```

Show `/tmp` variant if convenient. Mention `open`/`dup2`/`close` and that
unused pipe ends are closed so EOF works.

## 4. Background jobs (1 min)

```text
sleep 60 &
jobs
kill %1
jobs
sleep 60 &
```

Leave one running, show `[1] Done` notice appearing at the prompt when it
finishes (start `sleep 3 &` for this moment).

## 5. Signals and job control (1.5 min)

```text
sleep 30
```
Press Ctrl+C → shell is alive; run `echo survived`.

```text
sleep 30
```
Press Ctrl+Z → `[1] Stopped`. Then:

```text
jobs
bg 1
jobs
fg 1
```
Press Ctrl+C again → back to prompt.

Say: each job gets its own process group and the terminal is handed over with
`tcsetpgrp`, so terminal signals go to the job, not the shell. `SIGCHLD` is
delivered through a self-pipe and reaped in the main loop.

## 6. Monitoring (1.5 min)

```text
monitor
monitor --pid $$
ps
pinfo $$
```

Better: run `yes > /dev/null &` first, then `pinfo <pid>` to show ~100% CPU in
the 200 ms sample, then `kill %n`.

Point out `/proc/[pid]/stat` parsing after the last `)` and the difference
between cumulative CPU time and sampled utilization.

## 7. History and exit (30 s)

```text
history
!2
exit
```

## 8. Closing statement (30 s)

- User-space shell using kernel services; kernel still schedules and manages
  memory.
- Deliberate subset: no readline, no globbing, no `&&`/`;`.
- Full list of tests: `docs/test_cases.md`; viva answers: `docs/viva_qa.md`.

## Recording checklist

- [ ] Terminal shows prompt `user@minishell:...$`
- [ ] No compiler warnings during build
- [ ] `jobs` output visible after Ctrl+Z and after `bg`
- [ ] `pinfo` shows both `CPU time` and `CPU now`
- [ ] End with a clean `exit` (no leftover jobs)
