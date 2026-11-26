# FAQ

**Is this a bash replacement?**
No. It implements a deliberate subset: process creation, pipelines, redirection,
job control, signals and a `/proc` monitor. Line editing, globbing, variable
expansion and command chaining are out of scope (see Limitations in the README).

**Is it an operating system?**
No. It is a user-space program that uses Linux kernel services through POSIX
calls. The kernel still creates processes, schedules them and manages memory.

**Why can't I install it with npm, pip, brew or apt?**
Those package managers distribute scripts or libraries wrapped in metadata for
ecosystems this shell does not belong to. It is a compiled Linux executable
that calls `fork`, `execvp`, `pipe`, `dup2`, `sigaction` and reads `/proc`
directly. The curl installer builds it from source in seconds; a package manager
would add a maintainer and a layer without changing what the binary does.
If you want a system-wide package, build the tarball with
`cpack`/`cmake --install` yourself — nothing in the code blocks that.

**Does it work on Windows?**
Only inside WSL2. `fork`, process groups, terminal ownership and `/proc` are
Linux concepts. From PowerShell:

```powershell
wsl --install -d Ubuntu
```

Then run the curl installer inside Ubuntu.

**Does it work on macOS?**
No. `fork` exists but `/proc` does not, process-group terminal control differs,
and the monitor depends on Linux files. Use a VM, Docker, or a Linux machine.

**Why `unsafe` mechanisms instead of a library?**
That is the point of the project. Using a wrapper library that hides `fork`,
`waitpid` or `pipe` would be the same mistake as using `system()`.

**Why does Ctrl+Z do nothing in my IDE/pipe test?**
Job control is only enabled when stdin is a terminal (`isatty`). Piped input,
CI runners and some IDE consoles are not terminals, so the shell runs as a
non-interactive shell on purpose.

**Why does Ctrl+C at the prompt print a new prompt instead of exiting?**
The shell installs its own `SIGINT` handler; the handler only writes a byte to
the self-pipe. The line reader wakes on that byte, throws away whatever partial
line was typed, and draws a fresh prompt. Ctrl+C while a job is in the
foreground never reaches the shell, because the terminal belongs to the job's
process group.

**How do I see the demo without installing?**
Read `docs/test_cases.md` for recorded results and `docs/demo_script.md` for the
exact command sequence used in the video.

**Where do background jobs go when I exit?**
They receive `SIGHUP`, so the shell does not leave orphaned work behind.

**Can I contribute or extend it?**
Read `RULES.md` first: commit format, semver tags, signal-handler rules, fd
discipline, and the testing expectations. `docs/architecture.md` maps every file
to its job.

**Why is `monitor` showing cumulative CPU time in the table?**
Because a single `/proc` sample cannot produce a rate. `pinfo` takes a second
sample 200 ms later and reports `CPU now`, which is a real percentage.
