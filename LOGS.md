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
