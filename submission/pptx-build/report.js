// College submission report: Mini Linux Shell with Process Manager
const fs = require('fs');
const path = require('path');
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell, ImageRun,
  AlignmentType, LevelFormat, ExternalHyperlink, TableOfContents, HeadingLevel,
  BorderStyle, WidthType, ShadingType, PageBreak, Header, Footer, PageNumber,
} = require('docx');

const GIT_URL = 'https://github.com/Gunjan00001/Mini-Linux-Shell-with-Process-Manager';
const BODY = 'Times New Roman';
const HEAD = 'Arial';
const MONO = 'Courier New';

// ---------- helpers ----------
const p = (text, opts = {}) => new Paragraph({
  spacing: { after: 160, line: 300 },
  alignment: AlignmentType.JUSTIFIED,
  children: runs(text),
  ...opts.para,
});

// mini markup: **bold**, `mono`, ~~italic~~
function runs(text, base = {}) {
  const out = [];
  const re = /(\*\*[^*]+\*\*|`[^`]+`|~~[^~]+~~)/g;
  let last = 0, m;
  while ((m = re.exec(text))) {
    if (m.index > last) out.push(new TextRun({ text: text.slice(last, m.index), font: BODY, ...base }));
    const tok = m[0];
    if (tok.startsWith('**')) out.push(new TextRun({ text: tok.slice(2, -2), bold: true, font: BODY, ...base }));
    else if (tok.startsWith('`')) out.push(new TextRun({ text: tok.slice(1, -1), font: MONO, ...base }));
    else out.push(new TextRun({ text: tok.slice(2, -2), italics: true, font: BODY, ...base }));
    last = m.index + tok.length;
  }
  if (last < text.length) out.push(new TextRun({ text: text.slice(last), font: BODY, ...base }));
  return out;
}

const h1 = (t) => new Paragraph({ heading: HeadingLevel.HEADING_1, children: [new TextRun(t)] });
const h2 = (t) => new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun(t)] });
const bullet = (t) => new Paragraph({ numbering: { reference: 'bullets', level: 0 }, spacing: { after: 80 }, children: runs(t) });
const num = (ref, t) => new Paragraph({ numbering: { reference: ref, level: 0 }, spacing: { after: 80 }, children: runs(t) });
const pb = () => new Paragraph({ children: [new PageBreak()] });

const border = { style: BorderStyle.SINGLE, size: 1, color: 'BBBBBB' };
const cellBorders = { top: border, bottom: border, left: border, right: border };

function table(headers, rows, widths) {
  const total = 9360;
  const cw = widths || headers.map(() => Math.floor(total / headers.length));
  const headRow = new TableRow({
    tableHeader: true,
    children: headers.map((h, i) => new TableCell({
      borders: cellBorders, width: { size: cw[i], type: WidthType.DXA },
      shading: { fill: 'E8F0E4', type: ShadingType.CLEAR },
      children: [new Paragraph({ children: [new TextRun({ text: h, bold: true, font: HEAD, size: 20 })] })],
    })),
  });
  return new Table({
    columnWidths: cw,
    margins: { top: 80, bottom: 80, left: 140, right: 140 },
    rows: [headRow, ...rows.map(r => new TableRow({
      children: r.map((c, i) => new TableCell({
        borders: cellBorders, width: { size: cw[i], type: WidthType.DXA },
        children: [new Paragraph({ children: runs(c, { size: 20 }) })],
      })),
    }))],
  });
}

const codeBlock = (lines) => lines.map(l => new Paragraph({
  spacing: { after: 0, line: 260 },
  shading: { fill: 'F4F4F0', type: ShadingType.CLEAR },
  children: [new TextRun({ text: l, font: MONO, size: 19 })],
}));

const SHOTS = path.join(__dirname, '..', '..', 'docs', 'screenshots');
const figure = (file, caption) => [
  new Paragraph({
    alignment: AlignmentType.CENTER,
    spacing: { before: 140, after: 60 },
    children: [new ImageRun({
      type: 'png',
      data: fs.readFileSync(path.join(SHOTS, file)),
      transformation: { width: 600, height: 375 },
    })],
  }),
  new Paragraph({
    alignment: AlignmentType.CENTER,
    spacing: { after: 220 },
    children: [new TextRun({ text: caption, italics: true, font: BODY, size: 18, color: '555555' })],
  }),
];

// ---------- front matter ----------
const cover = [
  new Paragraph({ spacing: { before: 2200 }, alignment: AlignmentType.CENTER, children: [
    new TextRun({ text: 'A PROJECT REPORT', font: HEAD, size: 26, bold: true, color: '555555' }),
  ]}),
  new Paragraph({ spacing: { before: 300, after: 200 }, alignment: AlignmentType.CENTER, children: [
    new TextRun({ text: 'MINI LINUX SHELL', font: HEAD, size: 52, bold: true }),
  ]}),
  new Paragraph({ spacing: { after: 300 }, alignment: AlignmentType.CENTER, children: [
    new TextRun({ text: 'WITH PROCESS MANAGER', font: HEAD, size: 40, bold: true, color: '333333' }),
  ]}),
  new Paragraph({ spacing: { after: 1600 }, alignment: AlignmentType.CENTER, children: [
    new TextRun({ text: 'A shell implemented from scratch in C++17: processes, pipes, job control, signals and /proc monitoring', italics: true, font: BODY, size: 24 }),
  ]}),
  ...[
    'Submitted by:  **[Student Name]**        Roll No: **[Roll No]**',
    '**[Class/Course]**  |  Semester [Semester]',
    '**[College Name]**',
    'Under the guidance of:  **[Guide Name]**',
  ].map(t => new Paragraph({ spacing: { after: 200 }, alignment: AlignmentType.CENTER, children: runs(t, { size: 24 }) })),
  new Paragraph({ spacing: { before: 1400 }, alignment: AlignmentType.CENTER, children: [
    new TextRun({ text: 'Academic Year: [Academic Year]', font: BODY, size: 22, color: '555555' }),
  ]}),
  pb(),
];

const certificate = [
  h1('Certificate'),
  p('This is to certify that **[Student Name]**, Roll No. **[Roll No]**, of **[Class/Course]**, **[College Name]**, has successfully completed the mini project titled **"Mini Linux Shell with Process Manager"** under my guidance, in partial fulfilment of the requirements of the **[Course/Degree Name]** curriculum for the academic year **[Academic Year]**.'),
  p('The project implements an interactive Linux shell in C++17 that demonstrates the core operating system concepts of process creation, pipelines, I/O redirection, terminal job control, signal handling and filesystem based process monitoring.'),
  new Paragraph({ spacing: { before: 1200 }, children: [] }),
  new Paragraph({ alignment: AlignmentType.LEFT, spacing: { after: 400 }, children: [new TextRun({ text: 'Signature of the Guide:  ____________________', font: BODY, size: 22 })] }),
  new Paragraph({ children: [new TextRun({ text: '[Guide Name]', font: BODY, size: 22 })] }),
  pb(),
];

const acknowledgement = [
  h1('Acknowledgement'),
  p('I sincerely thank my project guide, **[Guide Name]**, for the continuous guidance and for pointing out the design flaws that made this shell sturdier. I thank the faculty of the Department of **[Department Name]**, **[College Name]**, for the Operating Systems course that this project puts into practice.'),
  p('I also thank the open source community: the FreeBSD sh, GNU Bash and the Linux man pages project taught me everything a shell is expected to do, and this project is my attempt to explain those mechanisms by building them.'),
  pb(),
];

const abstract = [
  h1('Abstract'),
  p('This report presents **Mini Linux Shell with Process Manager**, an interactive Linux shell written in **C++17** that implements the interesting parts of a shell directly on POSIX interfaces instead of hiding them behind a library. The shell supports external program execution via `fork()`/`execvp()`/`waitpid()`, pipelines of any length, input/output/append redirection with `dup2()`, background jobs with job ids and completion notices, complete terminal job control using a process group for each job and `tcsetpgrp()`, and zombie free child reaping driven by a `SIGCHLD` self pipe. A process monitor reads the Linux `/proc` filesystem to report process state, CPU time, memory and command lines, both for the whole system (`monitor`) and for a single PID (`pinfo`).'),
  p('The parser is a pure function, which makes it unit testable without a terminal; behavioral job control paths are verified by a Python pseudo terminal smoke test, and every push is built and tested by continuous integration. All 16 mandatory behavioral tests, 25 edge cases and the automated suites pass.'),
  p(`**Keywords:** operating systems, shell, fork, exec, pipes, job control, signals, /proc, C++17. Repository: ${GIT_URL}`),
  pb(),
];

const toc = [
  h1('Table of Contents'),
  new TableOfContents('Table of Contents', { hyperlink: true, headingStyleRange: '1-2' }),
  pb(),
];

// ---------- chapters ----------
const ch1 = [
  h1('1. Introduction'),
  h2('1.1 Background'),
  p('A **shell** is the program standing between the user and the operating system kernel: it prints a prompt, reads a command line, and asks the kernel to execute the requested programs. Every command the user types becomes, on the inside, a chain of operating system calls: process creation, program replacement, descriptor manipulation, signal delivery and process state inspection.'),
  p('Modern shells such as Bash implement all of this, but a student reading Bash source learns more about compatibility tables than about the mechanisms themselves. This project therefore builds a **teaching grade shell**: small enough to read entirely, and honest enough to show every system call it makes.'),
  h2('1.2 Objectives'),
  bullet('Implement process creation and program execution from scratch: `fork()`, `execvp()`, `waitpid()`.'),
  bullet('Support pipelines of any length and I/O redirection (`<`, `>`, `>>`) through `pipe()` and `dup2()`.'),
  bullet('Provide real terminal job control: background jobs, process groups, Ctrl+C / Ctrl+Z, `fg` and `bg`.'),
  bullet('Reap finished children without zombies and without unsafe work inside a signal handler (SIGCHLD self pipe).'),
  bullet('Build a process manager that reports whole system and individual process information by reading `/proc`.'),
  bullet('Keep the code testable: a pure parser, unit tests, a pseudo terminal smoke test and continuous integration.'),
  h2('1.3 Platform'),
  p('The shell targets modern Linux distributions (Ubuntu 22.04+, Debian 12+, Fedora, Arch) and requires a C++17 compiler, CMake 3.16+ and git. Windows users run it inside WSL2; macOS is not supported because the project depends on `fork()`, `waitpid()` and `/proc` semantics.'),
  pb(),
];

const ch2 = [
  h1('2. Problem Statement and Scope'),
  h2('2.1 Problem statement'),
  p('Design and implement an interactive Linux shell that demonstrates the operating system mechanisms hidden inside everyday commands, together with a process monitoring tool built on the `/proc` filesystem.'),
  h2('2.2 In scope'),
  bullet('Interactive command execution of external programs, with quoted arguments.'),
  bullet('Twelve builtin commands: `cd`, `pwd`, `exit`, `jobs`, `history`, `kill`, `fg`, `bg`, `ps`, `pinfo`, `monitor`, `help`.'),
  bullet('Pipelines (`a | b | c`), redirection (`<`, `>`, `>>`), background jobs (`cmd &`) and history recall (`!N`).'),
  bullet('Signal safe child reaping; the shell itself survives Ctrl+C.'),
  bullet('Systemwide process table and detailed views for individual processes derived from `/proc`.'),
  h2('2.3 Deliberately out of scope'),
  bullet('No globbing (`*.txt`), variable expansion, `;`, `&&`, `||` or subshells: command substitution is a parser problem, not an OS problem.'),
  bullet('No line editing (arrow keys, tab completion) or persistent history across sessions.'),
  bullet('Builtins run inside the shell process, so they cannot appear inside a pipeline and ignore redirection.'),
  bullet('Linux only, by design: POSIX process semantics are the subject of the project.'),
  pb(),
];

const ch3 = [
  h1('3. System Architecture'),
  h2('3.1 Process model'),
  ...codeBlock([
    '              user',
    '               |',
    '               v',
    'Shell loop: prompt, read_line (select)',
    '               |',
    '               v',
    'Parser: quotes, pipes, redirection, background',
    '               |',
    '               v',
    'Executor: builtins, fork/execvp, pipelines',
    '       |            |             |',
    '       v            v             v',
    '  Job table        IPC         Terminal',
    '  jobs,reap        dup2        pgid, tcsetpgrp',
    '       |            |             |',
    '       +            +             +',
    '                    |',
    '                    v',
    '             fork / execvp',
    '                    |',
    '                    v',
    '              Linux kernel',
    '                    |',
    '                    v',
    '             /proc filesystem',
    '                    |',
    '                    v',
    '       Monitor: monitor, ps, pinfo',
  ]),
  new Paragraph({ spacing: { after: 200 }, children: [] }),
  p('The pipeline is simple and one directional: **prompt → parser → executor → fork/execvp → kernel → /proc → monitor**. The only feedback edge is the signal self pipe, which lets job completion notices appear while the user is idle at the prompt.'),
  h2('3.2 Components'),
  table(
    ['Component', 'Source', 'Responsibility'],
    [
      ['Shell loop', '`shell/`', 'prompt, select driven line reader, job notices'],
      ['Parser', '`parser/`', 'pure tokenizer: quotes, `|`, `<`, `>`, `>>`, `&` → `ParsedLine`'],
      ['Executor', '`executor/`', 'builtin dispatch, fork/exec pipelines, foreground wait'],
      ['Job table', '`process/`', 'job ids, states, reaping, SIGHUP on exit'],
      ['Terminal', '`process/`', 'shell pgid, tcsetpgrp handoff, no operation without tty'],
      ['IPC', '`ipc/`', 'open/dup2/close for redirection'],
      ['Signals', '`signals/`', 'sigaction handlers, self pipe, child reset'],
      ['Monitor', '`monitor/`', '/proc parsing and report commands'],
      ['History', '`history/`', 'in memory command list'],
    ],
    [2000, 1600, 5760]),
  h2('3.3 Data model'),
  p('The parser and executor communicate through a single structure, `ParsedLine`, which contains a vector of pipeline stages (`Command`), a background flag and an error field. The parser never touches the operating system; the executor never parses strings again. A `Job` records the job id, every PID of every stage, the command text and a state of Running, Stopped or Done.'),
  pb(),
];

const ch4 = [
  h1('4. Design and Implementation'),
  h2('4.1 Foreground command execution'),
  num('exec-flow', '**Builtins first:** `cd`, `pwd`, `jobs` and friends run inside the shell process: `cd` must change the shell\u2019s own working directory.'),
  num('exec-flow', '**fork():** the child resets its signal dispositions to the defaults, joins a new process group when job control is available, and wires pipe and redirection file descriptors with `dup2()`.'),
  num('exec-flow', '**execvp():** the child\u2019s image is replaced by the requested program searched on PATH; a failed `execvp()` prints the reason and exits the child with status 127.'),
  num('exec-flow', '**Parent:** closes its unused pipe ends, hands the terminal to the child\u2019s process group with `tcsetpgrp()`, and waits with `waitpid(pid, WUNTRACED)`.'),
  num('exec-flow', '**Take back:** when the child exits or stops, the parent reclaims the terminal; a stopped child becomes a `Stopped` job.'),
  h2('4.2 Pipelines'),
  p('One `pipe()` per gap. Stage *i* reads from the previous pipe\u2019s read end and writes into the next pipe\u2019s write end; every end a stage does not use is closed before `execvp()`, so EOF propagates correctly and the pipeline terminates. The foreground waiter collects every stage before returning to the prompt.'),
  h2('4.3 Background jobs'),
  p('Background jobs follow the same fork path, but the parent does not wait: the job (all stage PIDs) is registered in the job table under a new id and the prompt returns immediately. At the next main loop iteration, `waitpid(-1, WNOHANG | WUNTRACED | WCONTINUED)` collects finished children and prints `[n] Done` notices.'),
  h2('4.4 The SIGCHLD self pipe'),
  p('Inside a signal handler only async signal safe work is legal. The handler therefore does exactly one thing: **write one byte to a pipe**. The shell\u2019s `select()` waits on stdin *and* that pipe simultaneously, so reaping, job table updates and notice printing all happen in normal code while the user sits at the prompt. One mechanism serves two purposes: the idle prompt wait and the safe completion notification.'),
  h2('4.5 Terminal job control'),
  p('When stdin is a real terminal, the shell puts itself in its own process group and hands terminal ownership to the foreground job\u2019s group via `tcsetpgrp()`, so Ctrl+C and Ctrl+Z are delivered to the job, never to the shell. `fg` regrants the terminal and sends `SIGCONT`; `bg` only sends `SIGCONT`. Without a tty the shell skips all of this and children share its group, the behaviour of a noninteractive shell.'),
  h2('4.6 Redirection'),
  p('For `<`, `>` and `>>` the target file is opened with `open()` using the appropriate flags (`O_RDONLY`, `O_TRUNC`, `O_APPEND`) and moved onto file descriptor 0 or 1 with `dup2()` inside the child, before `execvp()`. Unused descriptors are closed in both parent and children so no fd leaks survive a pipeline (verified: the shell\u2019s fd count is unchanged after 15 pipelines).'),
  h2('4.7 Process monitor'),
  p('`/proc/[pid]/stat` is parsed **starting after the last `)`**, because the comm field may contain spaces and parentheses. Fields 14/15 give cumulative user/system CPU ticks (converted with `sysconf(_SC_CLK_TCK)`), field 20 the thread count, fields 23/24 the virtual size and resident set (pages to bytes via `_SC_PAGESIZE`). `cmdline` is read as NUL separated bytes and rendered with spaces. Because CPU utilization is a rate, the detail views (`pinfo`, `monitor --pid`) take a second snapshot 200 ms later and report the tick delta as a percentage, while the table view stays cumulative. `/proc/meminfo` and `/proc/uptime` supply the system summary header.'),
  h2('4.8 Error handling'),
  bullet('Every fallible call (`fork`, `pipe`, `open`, `dup2`, `execvp`, `kill`, `chdir`, `tcsetpgrp`) reports through `perror()` or a formatted message; the shell always keeps running.'),
  bullet('`waitpid()` and `select()` retry on `EINTR`, which Ctrl+C would otherwise trigger.'),
  bullet('On exit, background jobs receive `SIGHUP`, so the shell never leaves work running behind it.'),
  pb(),
];

const ch5 = [
  h1('5. System Calls Used'),
  p('Every OS operation in the project goes through a POSIX call. The map from feature to interface:'),
  table(
    ['Call / interface', 'Where', 'Why'],
    [
      ['`fork()`', 'executor', 'create the child that will run the program'],
      ['`execvp()`', 'executor', 'replace the child image, search PATH'],
      ['`waitpid()`', 'executor, job_manager', 'collect exit/stop status, reap zombies'],
      ['`pipe()`', 'executor', 'connect stdout of stage i to stdin of stage i+1'],
      ['`dup2()`', 'ipc, executor', 'move pipe ends and files onto fd 0/1'],
      ['`open()`', 'ipc', 'open `<`, `>`, `>>` targets'],
      ['`close()`', 'ipc, executor, signals', 'release unused pipe ends and files'],
      ['`kill()`', 'executor, job_manager', 'kill builtin, SIGCONT for fg/bg, SIGHUP at exit'],
      ['`sigaction()` / `signal()`', 'signals', 'install handlers; ignore SIGQUIT/SIGTSTP/SIGTTIN/SIGTTOU in the shell'],
      ['`setpgid()`', 'executor, terminal', 'put jobs in their own process group'],
      ['`tcsetpgrp()` / `tcgetpgrp()`', 'terminal', 'hand over / verify ownership of the terminal'],
      ['`isatty()`', 'terminal', 'job control only when stdin is a terminal'],
      ['`select()`', 'shell', 'wait on stdin and the signal self pipe simultaneously'],
      ['`read()` / `write()`', 'shell, signals', 'line reader; async signal safe self pipe notification'],
      ['`chdir()` / `getcwd()`', 'executor, shell', 'cd and pwd builtins, prompt'],
      ['`getpid()`', 'executor, monitor', 'shell pid, ps parent filter'],
      ['`_exit()`', 'executor', 'child exit on failed redirection or execvp'],
      ['`fcntl()`', 'signals', 'nonblocking + close on exec self pipe ends'],
      ['`sysconf(_SC_CLK_TCK/_SC_PAGESIZE)`', 'monitor', 'ticks → seconds, pages → bytes'],
    ],
    [2900, 2200, 4260]),
  h2('5.1 /proc interfaces'),
  table(
    ['Path', 'Fields used', 'Feature'],
    [
      ['`/proc/[pid]/stat`', 'state(3), ppid(4), utime(14), stime(15), threads(20), vsize(23), rss(24)', 'monitor, ps, pinfo'],
      ['`/proc/[pid]/cmdline`', 'NUL separated argv', 'pinfo, monitor --pid'],
      ['`/proc/meminfo`', 'MemTotal, MemAvailable', 'monitor header'],
      ['`/proc/uptime`', 'first field', 'monitor header'],
    ],
    [2600, 4160, 2600]),
  h2('5.2 Signals handled'),
  table(
    ['Signal', 'Shell behaviour', 'Child behaviour'],
    [
      ['SIGCHLD', 'handler writes to self pipe; main loop reaps', 'reset to default'],
      ['SIGINT', 'ignored: Ctrl+C must not kill the shell', 'default: terminate'],
      ['SIGQUIT / SIGTSTP', 'ignored', 'default'],
      ['SIGTTIN / SIGTTOU', 'ignored (required for tcsetpgrp)', 'default'],
      ['SIGHUP', 'sent to jobs on shell exit', 'default: terminate'],
    ],
    [2000, 4360, 3000]),
  pb(),
];

const ch6 = [
  h1('6. Features and Usage'),
  h2('6.1 Command syntax'),
  table(
    ['Syntax', 'Meaning', 'Example'],
    [
      ['`cmd arg arg`', 'run a program', '`ls /tmp`'],
      ['"..." / \'...\'', 'one argument, spaces included', '`echo "Hello World"`'],
      ['`a | b | c`', 'pipeline, stdout to stdin', '`ls | grep txt | wc -l`'],
      ['`cmd < file`', 'stdin from file', '`sort < names.txt`'],
      ['`cmd > file`', 'stdout to file (truncate)', '`ls > list.txt`'],
      ['`cmd >> file`', 'stdout to file (append)', '`echo done >> list.txt`'],
      ['`cmd &`', 'background job', '`sleep 300 &`'],
      ['`!N`', 'run history entry N again', '`history`, then `!3`'],
    ],
    [2200, 3400, 3760]),
  h2('6.2 Builtin commands'),
  table(
    ['Command', 'What it does', 'Example'],
    [
      ['`cd [dir]`', 'change directory (default $HOME)', '`cd /tmp`'],
      ['`pwd`', 'print working directory', '`pwd`'],
      ['`exit`', 'leave the shell (background jobs get SIGHUP)', '`exit`'],
      ['`jobs`', 'list background and stopped jobs', '`jobs`'],
      ['`history`', 'numbered command history', '`history`'],
      ['`kill <pid|%job> [sig]`', 'send a signal (default SIGTERM)', '`kill %1 KILL`'],
      ['`fg <id>`', 'resume job in foreground with terminal', '`fg 1`'],
      ['`bg <id>`', 'resume job in background', '`bg 1`'],
      ['`ps`', 'processes created by this shell', '`ps`'],
      ['`pinfo <pid>`', 'detailed /proc information for one PID', '`pinfo 1`'],
      ['`monitor`', 'whole system process table + memory summary', '`monitor`'],
      ['`monitor --pid N`', 'same detail view as pinfo', '`monitor --pid $$`'],
    ],
    [2900, 4160, 2300]),
  h2('6.3 Sample session'),
  ...codeBlock([
    'gunjan@minishell:~$ ls | grep txt',
    'notes.txt',
    'gunjan@minishell:~$ sleep 30 &',
    '[1] Started  sleep 30  PID: 4210',
    'gunjan@minishell:~$ jobs',
    '[1] Running  sleep 30  PID: 4210',
    'gunjan@minishell:~$ monitor --pid 4210',
    '========== PROCESS INFO: 4210 ==========',
    'PID:        4210      PPID:   4000',
    'State:      S         Threads: 1',
    'CPU time:   0.02 s    CPU now: 0.0 % (200 ms)',
    'Virtual:    2.3 MB    Resident: 1.1 MB',
    'Cmdline:    sleep 30',
  ]),
  h2('6.4 Screenshots'),
  p('The screenshots below were captured from the running shell through a pseudo terminal. They show the real interaction: prompts, command output, job states and live /proc data.'),
  ...figure('quick_start.png', 'Figure 6.1  Quick start: execution, pipes, redirection and history'),
  pb(),
  ...figure('job_control.png', 'Figure 6.2  Job control: Ctrl+Z stops the job, bg resumes it, fg returns it, Ctrl+C interrupts it'),
  pb(),
  ...figure('monitor.png', 'Figure 6.3  Process monitor: ps and pinfo read live data from /proc'),
  pb(),
];

const ch7 = [
  h1('7. Testing and Results'),
  h2('7.1 Methodology'),
  bullet('**Unit tests** for the parser (12 cases: quoting, pipes, redirection, syntax errors) and the /proc monitor (parsing plus a live PID), run with CTest.'),
  bullet('**Pseudo terminal smoke test** (`tests/pty_smoke.py`): a Python script drives a real pty to verify Ctrl+C, Ctrl+Z, jobs, bg, fg and history overflow handling.'),
  bullet('**Resource checks:** no zombie children remain after reaping; the shell\u2019s file descriptor count is unchanged after 15 pipelines.'),
  bullet('**Continuous integration:** GitHub Actions builds the project and runs the unit tests on every push.'),
  h2('7.2 Results'),
  table(
    ['Test group', 'Cases', 'Result'],
    [
      ['Mandatory behavioral checklist (execution, jobs, redirection, signals, monitor, exit)', '16', 'PASS'],
      ['Edge cases (quotes, empty line, EINTR, dangling pipe, overflow, SIGHUP, ...)', '25', 'PASS'],
      ['Parser unit tests', '12', 'PASS'],
      ['Monitor unit tests', 'live + parsing', 'PASS'],
      ['pty smoke test', 'behavioral', 'PASS'],
      ['CI (ubuntu-latest)', 'build + tests', 'PASS'],
    ],
    [6360, 1500, 1500]),
  h2('7.3 Environment'),
  p('WSL2 Ubuntu 24.04, g++ 13.3.0, CMake 3.28.3. Ctrl+C / Ctrl+Z require a real terminal and are therefore exercised through a pseudo terminal; piped input deliberately disables terminal job control, matching the behaviour of noninteractive shells.'),
  h2('7.4 Test run screenshot'),
  ...figure('tests.png', 'Figure 7.1  Local test run: CTest and the pty smoke suite'),
  pb(),
];

const ch8 = [
  h1('8. Limitations'),
  bullet('No line editing: arrow keys, tab completion and persistent history are absent.'),
  bullet('Builtins run inside the shell process, so redirection is ignored for them and builtins cannot appear inside a pipeline (`history | wc -l` fails).'),
  bullet('No stderr redirection: `ls 2> err.txt` passes `2` as an argument.'),
  bullet('Backslashes are literal outside quotes; no globbing, variable expansion, `;`, `&&`, `||` or subshells.'),
  bullet('Background output can interleave with the prompt (no redraw).'),
  bullet('Job control requires a tty; piped stdin falls back to a noninteractive shell.'),
  h1('9. Future Scope'),
  bullet('Line editing with GNU Readline: arrow keys, tab completion, persistent history.'),
  bullet('Environment variable expansion, globbing and stderr redirection.'),
  bullet('Builtins in subshells so they can join pipelines.'),
  bullet('Noninteractive script mode supporting `;`, `&&` and `||`.'),
  bullet('A graphical monitor built on `/proc` as a companion view.'),
  pb(),
];

const ch9 = [
  h1('10. Conclusion'),
  p('The project demonstrates that a shell is not magic: it is `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `setpgid`, `tcsetpgrp`, `sigaction` and `/proc`, arranged carefully. Building each of these by hand, and making the shell survive Ctrl+C, reap its children without zombies, hand the terminal to jobs and take it back, converted the Operating Systems syllabus from definitions into working, testable code.'),
  p('The architecture stays honest by keeping the parser pure, the signal handler minimal and every failure path reported: 41 behavioral checks, 12 parser cases and a pseudo terminal smoke suite all pass, with continuous integration guarding every push. The result is a small program (nine modules, C++17, no dependencies beyond the POSIX interface) that can be read in an afternoon and defended line by line.'),
  h1('11. References'),
  num('refs', 'Linux man pages project: fork(2), exec(3), wait(2), pipe(2), dup(2), sigaction(2), setpgid(2), tcsetpgrp(3), proc(5).'),
  num('refs', 'Michael Kerrisk, *The Linux Programming Interface*, No Starch Press, 2010.'),
  num('refs', 'Abraham Silberschatz, Peter Galvin, Greg Gagne, *Operating System Concepts*, 10th ed., Wiley.'),
  num('refs', 'FreeBSD sh and GNU Bash sources: reference behaviours for job control.'),
  new Paragraph({ spacing: { before: 200 }, children: runs(`**Source repository:** `) }),
  new Paragraph({
    spacing: { after: 200 },
    children: [new ExternalHyperlink({
      children: [new TextRun({ text: GIT_URL, style: 'Hyperlink', font: MONO, size: 22 })],
      link: GIT_URL,
    })],
  }),
];

// ---------- document ----------
const doc = new Document({
  styles: {
    default: { document: { run: { font: BODY, size: 22 } } }, // 11pt body
    paragraphStyles: [
      { id: 'Heading1', name: 'Heading 1', basedOn: 'Normal', next: 'Normal', quickFormat: true,
        run: { size: 30, bold: true, color: '1F2933', font: HEAD },
        paragraph: { spacing: { before: 320, after: 200 }, outlineLevel: 0 } },
      { id: 'Heading2', name: 'Heading 2', basedOn: 'Normal', next: 'Normal', quickFormat: true,
        run: { size: 25, bold: true, color: '2E4A38', font: HEAD },
        paragraph: { spacing: { before: 240, after: 140 }, outlineLevel: 1 } },
    ],
  },
  numbering: {
    config: [
      { reference: 'bullets', levels: [{ level: 0, format: LevelFormat.BULLET, text: '•', alignment: AlignmentType.LEFT, style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
      ...['exec-flow', 'refs'].map(ref => ({
        reference: ref,
        levels: [{ level: 0, format: LevelFormat.DECIMAL, text: '%1.', alignment: AlignmentType.LEFT, style: { paragraph: { indent: { left: 720, hanging: 360 } } } }],
      })),
    ],
  },
  sections: [{
    properties: {
      page: {
        margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 },
        pageNumbers: { start: 1, formatType: 'decimal' },
      },
    },
    headers: {
      default: new Header({ children: [new Paragraph({
        alignment: AlignmentType.RIGHT,
        children: [new TextRun({ text: 'Mini Linux Shell with Process Manager', font: HEAD, size: 16, color: '888888' })],
      })] }),
    },
    footers: {
      default: new Footer({ children: [new Paragraph({
        alignment: AlignmentType.CENTER,
        children: [
          new TextRun({ text: 'Page ', font: BODY, size: 18 }),
          new TextRun({ children: [PageNumber.CURRENT], font: BODY, size: 18 }),
          new TextRun({ text: ' of ', font: BODY, size: 18 }),
          new TextRun({ children: [PageNumber.TOTAL_PAGES], font: BODY, size: 18 }),
        ],
      })] }),
    },
    children: [
      ...cover, ...certificate, ...acknowledgement, ...abstract, ...toc,
      ...ch1, ...ch2, ...ch3, ...ch4, ...ch5, ...ch6, ...ch7, ...ch8, ...ch9,
    ],
  }],
});

Packer.toBuffer(doc).then(buf => {
  const out = path.join(__dirname, '..', 'minishell_report.docx');
  fs.writeFileSync(out, buf);
  console.log('saved', out);
});
