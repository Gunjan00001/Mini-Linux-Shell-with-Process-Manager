# Work Log

Newest entries last. Each entry: what was done, why, and what was deliberately
left out. Commit short-hashes refer to this repository's history.

## 2026-11-23 — Project foundation

**Goal:** get the 7-day plan into a buildable skeleton inside `D:\OS project`,
with a real Linux toolchain (WSL2 Ubuntu 24.04, g++ 13.3, cmake 3.28).

- `27d9f1a` CMake project, `-Wall -Wextra -Wpedantic`, `src/` include root.
  Three targets: `minishell`, `parser_tests`, `monitor_tests`.
- `.gitignore` excludes `build/` so the repository stays source-only.
- `7eb0128` `.gitattributes` forces LF on source and docs. Without it, Windows
  `core.autocrlf` rewrites the tree to CRLF and every later diff looks huge.

## 2026-11-23 — Parser

- `022af96` One tokenizer pass: whitespace, single/double quotes, backslash
  inside double quotes, then the operators `|`, `<`, `>`, `>>`, `&`.
  Why not `split(' ')`: `echo "Hello World"` must stay one argument, and
  `>>` must not be read as two `>`.
- `848ae0b` 12 assert cases, including the ones that hurt later: unterminated
  quote, dangling pipe, `&` mid-line. Tests `#undef NDEBUG` before `<cassert>`
  because Release builds define `NDEBUG` and silently turn asserts into no-ops
  (this actually happened: first Release test run "passed" without testing).

## 2026-11-23 — Process bookkeeping

- `3c19757` Signals: handler only `write()`s a byte to a self-pipe. No
  containers, no allocation, no `waitpid()` in the handler. `select()` in the
  line reader watches stdin and the pipe, so a background job finishing while
  the user idles is noticed immediately instead of at the next Enter.
- `b7e5f10` Job table: `id -> pids, command, state`. Reaping uses
  `waitpid(-1, WNOHANG | WUNTRACED | WCONTINUED)` from the main loop only. A
  finished pipeline stage is removed from the job; the job is erased and a
  `Done` notice printed only when all its pids are gone.
- `205269f` History: `std::vector<std::string>`, skip consecutive duplicates,
  cap at 1000. No file persistence — not required.
- `07ba75a` Redirection: `open` + `dup2` + `close` in the child only. Append
  uses `O_APPEND`, truncate uses `O_TRUNC`.

## 2026-11-23 — Job control (a real defect, not a nicety)

- `bf1ca45` First implementation (children sharing the shell's process group)
  failed the pty test: Ctrl+Z did not stop the foreground job. Reason is
  POSIX, not a bug in the handler: a process group is *orphaned* when no member
  has a parent in the same session outside the group, and `SIGTSTP` is discarded
  for orphaned groups. In a real terminal the group is not orphaned, but the
  behavior is still unreliable and Ctrl+C hits every group member.
- Fix: proper terminal job control. Shell puts itself in its own pgid, children
  get their own pgid, the terminal is handed over with `tcsetpgrp()` for
  foreground jobs and taken back after `waitpid()`. `SIGTTIN`/`SIGTTOU` are
  ignored so the handoff is allowed. Without a tty (`isatty(0) == false`) all of
  this is a no-op, so piped usage keeps the simple inherited group.

## 2026-11-23 — Monitoring

- `4b57500` `/proc/[pid]/stat` is parsed from the last `)` onward. The `comm`
  field may contain spaces and parentheses; naive whitespace splitting breaks
  on e.g. `(my prog)`. Fields used: state, ppid, utime, stime, num_threads,
  vsize, rss. `cmdline` NULs become spaces. `monitor` adds a `meminfo`/`uptime`
  header. CPU utilization is intentionally not sampled: cumulative ticks are
  honest, a single-sample percentage is a lie.

## 2026-11-23 — Executor and shell loop

- `b7a9b43` Builtins run in-process (`cd` must change the shell's own cwd).
  External commands fork, reset signal dispositions, optionally `setpgid`,
  wire pipes and redirection, then `execvp`. Parent closes every unused pipe
  end. Failed `execvp` exits the child with 127 after printing `errno`.
- Multiple pipeline stages work for the same reason two do: one pipe per gap
  and a loop; there is no special case for exactly two commands.
- `e1d3455` Prompt `user@minishell:cwd$` with `$HOME` shortened to `~`. The
  line reader reads byte-by-byte through `select`, which is what makes job
  notices appear at the prompt. No readline: no arrow keys, no tab completion,
  no history navigation. Deliberate — a terminal UI library is not the point of
  the project.
- `314ae51` `main` installs signal handlers before taking terminal ownership
  (the `SIGTTOU` ignore must be in place first).
- `cbf84ef` pty smoke test drives the binary through a real pseudo-terminal:
  Ctrl+C, Ctrl+Z, `jobs`, `bg`, `fg`, Ctrl+C again. It caught the `bg` bug
  where the job was erased from the table instead of kept.

## 2026-11-23 — Verification

- 15 fd checks after repeated pipelines: `/proc/<shell>/fd` count unchanged.
- `sleep 0.2 &` followed by an idle second leaves no children (`ps --ppid`):
  the self-pipe path reaps before the prompt redraw.
- Clean rebuild with warnings enabled: zero warnings.
- `docs/test_cases.md` records every required test and the observed result.

## 2026-11-23 — Presentation polish

- Job notices showed the raw line, including the trailing `&` (`[1] Running
  sleep 30 &`). The executor now strips trailing `&`/spaces for the job table
  and the `Started`/`Done` messages, so they read like bash.

## 2026-11-23 — CI, viva, CPU sampling, history recall

- `e0bd7ee` Versioning documented as `vMAJOR.MINOR.BUGFIX` (third number counts
  bug fixes) after confusing it with git tags.
- `0b548fa` GitHub Actions CI: configure, build and `ctest` on ubuntu-latest.
  The repo now verifies itself on every push without relying on WSL.
- `24d3bf8` `docs/viva_qa.md`: 29 answers tied to specific files and system
  calls. Written for the viva, not as marketing.
- `d1ae792` `pinfo` / `monitor --pid` now report CPU utilization. Two stat
  snapshots 200 ms apart; delta ticks / `CLK_TCK` / interval. Verified against a
  busy `yes` process: 100.0 %. README/architecture/test docs updated so they no
  longer claim this is unsupported.
- `08ecf6d` `!N` recalls history entry N. Prints the expanded command and
  flushes stdout before running it, otherwise child output overtakes the echo.
  Unknown entries report `event not found` instead of executing anything.
- `docs/demo_script.md`: the exact recording sequence for the submission video,
  including the Ctrl+C/Ctrl+Z/bg/fg segment and the CPU sample.

## 2026-11-25 — Second bug hunt (v1.1.2)

Fuzzed 29 hostile inputs (unbalanced quotes, operator-only lines, overflow pids,
`kill 0`, `kill -1`, a 100 KB argument, an 81-stage pipeline). No crashes. Fixes
for the divergences found:

- `kill -9 pid` / `kill -KILL %job` now work; the leading `-` was previously
  parsed as part of the pid and rejected. Plain `kill pid [sig]` is unchanged.
- Ctrl+Z on a multi-stage foreground pipeline recorded only the first stage in
  the job table. `wait_foreground` now stores every pipeline pid, so `jobs`
  shows `PID: 459,460` and `fg` waits for the whole job. Verified: stopped
  pipeline is fully cleaned up on exit.
- `monitor` scanned `/proc` twice per invocation (once for the task count, once
  for the table). One scan now feeds both.
- Documented instead of changed: no stderr (`2>`) redirection, and backslashes
  are literal outside quotes. Both are parser-scope decisions, listed in
  README Limitations and `docs/test_cases.md`.

## 2026-11-25 — Bug hunt and fixes (v1.1.1)

Found by reproducing, not by reading:

- **`!N` overflow crashed the shell.** `!99999999999999999999` reached
  `std::stoul`, threw `std::out_of_range`, and aborted the process (core dump).
  Fix: digits longer than 9 are rejected and reported as `event not found`.
  Regression: pty smoke now sends the overflowing value and asserts the shell
  still answers the next command.
- **Background pipelines had one process group per stage.** Every stage child
  called `setpgid(0,0)`, so `sleep 30 | sleep 40 &` produced pgids `510` and
  `511`; `kill %1` killed only the first and `sleep 40` survived. Fix: stage 0
  creates the group, later stages join it (child and parent both call
  `setpgid` with stage 0's pid). Verified: both stages now show pgid 458 and
  `kill %1` leaves nothing behind.

Found by reading the code:

- Pipe read end leaked when `fork()` failed mid-pipeline (only the write end was
  closed). Both ends are closed now.
- Exit cleanup could not kill *stopped* jobs: a stopped process does not act on
  `SIGHUP` until continued. `terminate_all` now sends `killpg(SIGHUP)` then
  `killpg(SIGCONT)`, with per-pid fallback for jobs without their own group.
- `kill`, `fg`, `bg`, `pinfo`, `monitor --pid` used `atoi`, which is undefined
  behavior on overflow. Replaced with `strtol` + range checks; bad values now
  print `invalid pid` / `no such job`.
- `kill %n` and `bg %n` now signal the whole group with `killpg`, not just the
  leader.

Documented instead of "fixed": builtins ignore redirection and cannot run
inside pipelines; both are consequences of running builtins in the shell
process and are now listed in README Limitations and `docs/test_cases.md`.

## 2026-11-23 — Install path and documentation overhaul

- `install.sh`: clone into `~/.local/src`, Release build, install to
  `~/.local/bin`. No sudo, refuses non-Linux with a WSL2 pointer, prints the
  PATH line when needed, supports `--uninstall`. Tested end to end against the
  GitHub remote with a throwaway `HOME` (clone → build → run → uninstall).
- README rewritten as a user document, not a project note: requirements with
  WSL2 instructions, three install routes (curl one-liner, prebuilt release
  binary, git clone), quick start, full syntax/builtin/job-control/monitoring
  usage, troubleshooting table, uninstall, limitations, docs index.
- Prebuilt `minishell-linux-x86_64` built with `-static-libstdc++
  -static-libgcc`; `ldd` shows only libc, so it runs on any modern x86_64 glibc
  distro. Attached to the `v1.1.0` GitHub release.
- `docs/faq.md`: why no npm/pip/brew, Windows/macOS behavior, why raw syscalls,
  non-tty job control, CPU sampling.
- Deliberately not done: Homebrew tap, AUR/PPA packaging, self-hosted package
  repository. They need maintainer accounts and periodic rebuilds; the curl
  installer covers the same need without that upkeep.

## Versioning meaning (clarified)

The third number counts **bug fixes**, not releases: `vMAJOR.MINOR.BUGFIX`.
`v1.0.1` therefore means "core shell, one bug fix shipped". Rule recorded in
`RULES.md` §6.

## Deliberately not built

- Line editing / history navigation / tab completion.
- Environment expansion, globbing, `&&`, `||`, `;`, subshells.
- Sampled CPU utilization and a web dashboard (future work per spec).
- Persistent history, config file, custom prompt variables.

These are listed in `README.md` under Limitations so the boundary is explicit
in the viva.

## Next steps

- Add a CI workflow (build + unit tests on ubuntu-latest) if the repository is
  used for grading.
- Optional: two-snapshot CPU% in `pinfo` when a 100 ms window is acceptable.
