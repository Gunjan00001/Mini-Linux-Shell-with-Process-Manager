// Build the minishell college presentation: 14 HTML slides -> PPTX, then screenshot each slide for validation.
const fs = require('fs');
const path = require('path');
const { pathToFileURL } = require('url');
const pptxgen = require('pptxgenjs');
const html2pptx = require('./html2pptx');
const { chromium } = require('playwright');

const OUT = path.join(__dirname, 'slides');
fs.mkdirSync(OUT, { recursive: true });

const GIT = 'github.com/Gunjan00001/Mini-Linux-Shell-with-Process-Manager';
const SHOTS = path.join(__dirname, '..', '..', 'docs', 'screenshots');
const shot = (name) => pathToFileURL(path.join(SHOTS, name)).href;

// ---- shared CSS (terminal dark theme) ----
const css = `
html { background: #14181d; }
body { width: 720pt; height: 405pt; margin: 0; padding: 0; background: #14181d;
       font-family: Arial, sans-serif; color: #edeff2; display: flex; flex-direction: column; }
.bar { background: #1f2933; padding: 16pt 30pt 12pt 30pt; border-bottom: 4pt solid #4e9f3d; }
.bar h1 { margin: 0; font-size: 22pt; color: #edeff2; }
.bar p { margin: 2pt 0 0 0; font-size: 10pt; color: #98acb5; }
.content { padding: 16pt 34pt; display: flex; flex-direction: column; flex: 1; }
h2 { font-size: 15pt; color: #4e9f3d; margin: 0 0 6pt 0; }
p { margin: 4pt 0; font-size: 12.5pt; line-height: 1.35; }
ul { margin: 4pt 0 0 0; padding-left: 16pt; }
li { font-size: 12.5pt; line-height: 1.45; margin-bottom: 4pt; }
.mono { font-family: 'Courier New', monospace; color: #7ec98f; }
.dim { color: #98acb5; }
.code { background: #0c0f12; border: 1pt solid #2e3d46; border-radius: 6pt; padding: 10pt 14pt;
        font-family: 'Courier New', monospace; }
.code p { font-family: 'Courier New', monospace; font-size: 11pt; color: #b9e4a6; margin: 3pt 0; }
.col { display: flex; flex-direction: row; gap: 18pt; }
.panel { background: #1f2933; border-radius: 8pt; padding: 12pt 16pt; flex: 1; }
.panel h2 { margin-top: 0; }
.box { background: #1f2933; border: 1pt solid #3a4a54; border-radius: 6pt; padding: 8pt 10pt; }
.arrow { font-size: 13pt; color: #4e9f3d; padding: 0 2pt; }
.foot { padding: 0 34pt 10pt 34pt; display: flex; flex-direction: row; justify-content: space-between; }
.foot p { font-size: 8.5pt; color: #5c6f7a; }
.shot { width: 100%; border-radius: 6pt; }
`;

function slide(title, sub, body, footLeft) {
  return `<!DOCTYPE html><html><head><style>${css}</style></head><body>
<div class="bar"><h1>${title}</h1>${sub ? `<p>${sub}</p>` : ''}</div>
<div class="content">${body}</div>
<div class="foot"><p style="width: 170pt;">${footLeft || 'Mini Linux Shell with Process Manager'}</p><p style="width: 170pt; text-align: right;">[Student Name] &bull; [Roll No]</p></div>
</body></html>`;
}

const slides = [];

// ---- 1. Title ----
slides.push(`<!DOCTYPE html><html><head><style>${css}
.wrap { flex: 1; display: flex; flex-direction: column; justify-content: center; padding: 0 60pt; }
h1 { font-size: 34pt; margin: 0; color: #edeff2; }
.tagline { font-size: 14pt; color: #4e9f3d; margin: 8pt 0 0 0; }
.who { margin-top: 26pt; font-size: 12pt; color: #98acb5; line-height: 1.6; }
</style></head><body>
<div class="wrap">
  <div class="code" style="width: 340pt;"><p class="dim">gunjan@minishell:~$ <span style="color:#b9e4a6">./minishell</span></p></div>
  <h1>Mini Linux Shell<br/>with Process Manager</h1>
  <p class="tagline">Operating Systems Mini Project, implemented in C++17</p>
  <div class="who">
    <p><b>[Student Name]</b> &nbsp;&bull;&nbsp; Roll No: [Roll No] &nbsp;&bull;&nbsp; [Class/Course]</p>
    <p>[College Name]</p>
    <p>Guide: [Guide Name]</p>
  </div>
</div>
</body></html>`);

// ---- 2. Introduction & Objective ----
slides.push(slide('Introduction &amp; Objective', 'Why build a shell?',
`<div class="col">
 <div class="panel" style="flex: 1.1;">
  <h2>What is a shell?</h2>
  <p>The program between the user and the kernel: it reads a command line, and asks the OS to run it.</p>
  <p class="dim">Most shells hide the machinery. We show it.</p>
 </div>
 <div class="panel" style="flex: 1.3;">
  <h2>Objective</h2>
  <ul>
   <li>Implement the core OS mechanisms <b>from scratch</b>: <span class="mono">fork</span>, <span class="mono">execvp</span>, <span class="mono">waitpid</span>, pipes, <span class="mono">dup2</span> redirection</li>
   <li>Real terminal job control: process groups, <span class="mono">tcsetpgrp</span>, Ctrl+C / Ctrl+Z, <span class="mono">fg</span>/<span class="mono">bg</span></li>
   <li>Safe <span class="mono">SIGCHLD</span> reaping through a self pipe</li>
   <li>A process manager reading the Linux <span class="mono">/proc</span> filesystem</li>
  </ul>
 </div>
</div>`, 'Introduction'));

// ---- 3. Problem Statement & Scope ----
slides.push(slide('Problem Statement &amp; Scope', 'What is implemented, and what is deliberately not',
`<div class="col">
 <div class="panel" style="border-top: 3pt solid #4e9f3d;">
  <h2>In scope</h2>
  <ul>
   <li>Interactive Linux shell in C++17 using POSIX system calls directly</li>
   <li>Pipelines, I/O redirection, background jobs, full job control</li>
   <li>Signal safe child reaping; the shell survives Ctrl+C</li>
   <li>Monitoring the whole system and individual processes from <span class="mono">/proc</span></li>
  </ul>
 </div>
 <div class="panel" style="border-top: 3pt solid #5c6f7a;">
  <h2>Deliberately out of scope</h2>
  <ul>
   <li>No globbing (<span class="mono">*.txt</span>), variable expansion, <span class="mono">;</span> / <span class="mono">&amp;&amp;</span> / <span class="mono">||</span></li>
   <li>No tab completion or line editing</li>
   <li>Builtins cannot appear inside a pipeline</li>
   <li>Linux only, because POSIX semantics are the point of the project</li>
  </ul>
 </div>
</div>`, 'Problem Statement'));

// ---- 4. Features ----
slides.push(slide('Features', 'Everything works end to end',
`<div class="col">
 <div class="panel">
  <h2>Execution &amp; builtins</h2>
  <ul>
   <li>External programs via <span class="mono">execvp</span>; quoted arguments</li>
   <li>12 builtins: <span class="mono">cd pwd exit jobs history kill fg bg ps pinfo monitor help</span></li>
   <li><span class="mono">!N</span> runs history entry N again</li>
  </ul>
  <h2>Pipelines &amp; redirection</h2>
  <ul>
   <li>Any number of stages: <span class="mono">a | b | c</span></li>
   <li><span class="mono">&lt; in</span>, <span class="mono">&gt; out</span>, <span class="mono">&gt;&gt; append</span></li>
  </ul>
 </div>
 <div class="panel">
  <h2>Jobs, signals &amp; monitoring</h2>
  <ul>
   <li><span class="mono">cmd &amp;</span> background jobs with job ids and <span class="mono">[n] Done</span> notices</li>
   <li>Each job gets its own process group; terminal handoff; Ctrl+C / Ctrl+Z / <span class="mono">fg</span> / <span class="mono">bg</span></li>
   <li><span class="mono">SIGCHLD</span> reaping via a self pipe</li>
   <li><span class="mono">monitor</span>, <span class="mono">pinfo</span>, <span class="mono">ps</span> read <span class="mono">/proc</span>: state, CPU, memory, cmdline</li>
  </ul>
 </div>
</div>`, 'Features'));

// ---- 5. Architecture ----
slides.push(slide('System Architecture', 'prompt &rarr; parser &rarr; executor &rarr; kernel &rarr; /proc',
`<div style="display: flex; flex-direction: row; align-items: center; justify-content: center; gap: 4pt;">
 <div class="box"><p>User</p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>Shell loop<br/><span class="dim" style="font-size:9pt">select()</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>Parser<br/><span class="dim" style="font-size:9pt">ParsedLine</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>Executor<br/><span class="dim" style="font-size:9pt">builtins / fork</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>Linux kernel<br/><span class="dim" style="font-size:9pt">fork / exec</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>/proc</p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>Monitor</p></div>
</div>
<div style="display: flex; flex-direction: row; justify-content: center; margin-top: 14pt;">
 <div class="box" style="border-color: #4e9f3d;"><p>Signal handler: <span class="mono">SIGCHLD</span> &#8594; self pipe (1 byte)</p></div>
 <p class="arrow">&#8593;</p>
 <div class="box"><p>Job notices while idle at the prompt: <span class="mono">[1] Done</span></p></div>
</div>
<div style="margin-top: 14pt;">
 <p class="dim">The parser is a pure function, unit tested without a terminal. The executor never parses strings again.</p>
 <p class="dim">Under the executor: JobManager (jobs, reaping) &bull; IPC (dup2 redirection) &bull; Terminal (pgid / tcsetpgrp).</p>
</div>`, 'Architecture'));

// ---- 6. Modules ----
slides.push(slide('Module Breakdown', '9 small components, one clear job each',
`<div class="col">
 <div class="panel" style="flex: 1;">
  <ul>
   <li><b>Shell loop</b>: <span class="mono">shell/</span>, prompt, select driven line reader</li>
   <li><b>Parser</b>: <span class="mono">parser/</span>, quotes, |, &lt;, &gt;, &gt;&gt;, &amp; &rarr; <span class="mono">ParsedLine</span></li>
   <li><b>Executor</b>: <span class="mono">executor/</span>, builtin dispatch, fork/exec pipelines</li>
   <li><b>Job table</b>: <span class="mono">process/</span>, job ids, states, reaping, SIGHUP at exit</li>
   <li><b>Terminal</b>: <span class="mono">process/</span>, shell pgid, <span class="mono">tcsetpgrp</span> handoff</li>
  </ul>
 </div>
 <div class="panel" style="flex: 1;">
  <ul>
   <li><b>IPC</b>: <span class="mono">ipc/</span>, <span class="mono">open</span>/<span class="mono">dup2</span>/<span class="mono">close</span> for redirection</li>
   <li><b>Signals</b>: <span class="mono">signals/</span>, <span class="mono">sigaction</span>, self pipe, child reset</li>
   <li><b>Monitor</b>: <span class="mono">monitor/</span>, /proc parsing, monitor/ps/pinfo</li>
   <li><b>History</b>: <span class="mono">history/</span>, in memory command list</li>
  </ul>
  <p class="dim" style="margin-top: 8pt;">Data contract: <span class="mono">ParsedLine { stages, background, ok }</span> is the only thing passed parser &rarr; executor.</p>
 </div>
</div>`, 'Modules'));

// ---- 7. Process creation ----
slides.push(slide('Key Design: Process Creation', 'fork / execvp / waitpid, the heart of every shell',
`<ol>
 <li><b><span class="mono">fork()</span></b>: child resets signal dispositions, joins its own process group (<span class="mono">setpgid</span>)</li>
 <li><b>Wire fds</b>: pipe ends and redirection files moved onto fd 0/1 with <span class="mono">dup2</span>; unused ends <span class="mono">close</span>d</li>
 <li><b><span class="mono">execvp()</span></b>: child image replaced, searched on <span class="mono">PATH</span>; failure exits with status 127</li>
 <li><b>Parent</b>: closes its pipe ends, hands the terminal to the child's pgid, waits with <span class="mono">waitpid(pid, WUNTRACED)</span></li>
 <li><b>Take back</b>: shell reclaims the terminal; a stopped child becomes a <span class="mono">Stopped</span> job</li>
</ol>
<div class="code" style="margin-top: 8pt;"><p>Pipeline: one <span class="mono">pipe()</span> per gap; each stage reads the previous stage and writes the next</p>
<p>Unused ends closed before <span class="mono">execvp</span>, so EOF propagates correctly through the pipeline</p></div>`, 'Process Creation'));

// ---- 8. Job control ----
slides.push(slide('Key Design: Terminal Job Control', 'Ctrl+C must hit the job, never the shell',
`<div class="col">
 <div class="panel" style="flex: 1.2;">
  <ul>
   <li>Shell puts itself in its own process group at startup</li>
   <li>Each job gets its <b>own process group</b> (<span class="mono">setpgid</span>)</li>
   <li><span class="mono">tcsetpgrp()</span> hands the terminal to the foreground job, so Ctrl+C / Ctrl+Z are delivered to the job, not the shell</li>
   <li><span class="mono">fg</span>: grant terminal + <span class="mono">SIGCONT</span>; <span class="mono">bg</span>: <span class="mono">SIGCONT</span> only</li>
   <li>No tty? (piped stdin) &rarr; shell skips job control, children share its group</li>
  </ul>
 </div>
 <div class="panel" style="flex: 1;">
  <h2>Live behavior</h2>
  <div class="code">
   <p><span class="dim">$</span> sleep 30 &nbsp;&nbsp;<span style="color:#5c6f7a">then Ctrl+Z</span></p>
   <p><span class="dim">[1] Stopped&nbsp; sleep 30&nbsp; PID: 4210</span></p>
   <p><span class="dim">$</span> bg 1 &nbsp;&nbsp;<span class="dim">&rarr;</span> resumes in background</p>
   <p><span class="dim">$</span> fg 1 &nbsp;&nbsp;<span class="dim">&rarr;</span> back to foreground</p>
  </div>
 </div>
</div>`, 'Job Control'));

// ---- 9. Self pipe ----
slides.push(slide('Key Design: SIGCHLD Self Pipe', 'Reaping zombies safely',
`<div style="display: flex; flex-direction: row; align-items: center; justify-content: center; gap: 6pt;">
 <div class="box"><p>SIGCHLD<br/><span class="dim" style="font-size:9pt">handler</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>write() 1 byte<br/><span class="dim" style="font-size:9pt">async signal safe</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>self pipe</p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>select() wakes<br/><span class="dim" style="font-size:9pt">stdin + pipe</span></p></div><p class="arrow">&#8594;</p>
 <div class="box"><p>main loop reaps<br/><span class="dim" style="font-size:9pt">waitpid(WNOHANG)</span></p></div>
</div>
<div style="margin-top: 16pt;">
 <ul>
  <li><b>Why not reap inside the handler?</b> Only <span class="mono">write()</span> is async signal safe, so all <span class="mono">waitpid()</span> calls and job table updates happen in normal code</li>
  <li><b>Result:</b> <span class="mono">[n] Done</span> notices appear while the user is idle at the prompt; no zombies are ever left</li>
  <li><span class="mono">select()</span> also gives us the idle prompt wait: one mechanism, two jobs</li>
 </ul>
</div>`, 'Self Pipe'));

// ---- 10. Monitor ----
slides.push(slide('Process Monitor', 'Reading the kernel\u2019s /proc filesystem',
`<div class="col">
 <div class="panel" style="flex: 1.1;">
  <h2>What we read</h2>
  <ul>
   <li><span class="mono">/proc/[pid]/stat</span>: state, PPID, utime/stime, threads, vsize, RSS</li>
   <li><span class="mono">/proc/[pid]/cmdline</span>: NUL separated argv</li>
   <li><span class="mono">/proc/meminfo</span>, <span class="mono">/proc/uptime</span>: header summary</li>
  </ul>
  <p class="dim" style="margin-top: 6pt;">Parsed <b>after the last )</b> so process names with spaces and parentheses are safe. CPU % = tick delta between two snapshots 200 ms apart.</p>
 </div>
 <div style="flex: 1; display: flex; flex-direction: column; justify-content: center;">
  <img class="shot" src="${shot('monitor.png')}" style="width: 305pt; height: 191pt;"/>
  <p class="dim" style="font-size: 9pt; margin-top: 5pt;">pinfo output: live data from a running process</p>
 </div>
</div>`, 'Monitor'));

// ---- 11. Demo ----
slides.push(slide('Demo Session', 'Real captured sessions from the running shell',
`<div class="col" style="gap: 12pt;">
 <div style="flex: 1; display: flex; flex-direction: column;">
  <img class="shot" src="${shot('quick_start.png')}" style="width: 310pt; height: 194pt;"/>
  <p class="dim" style="font-size: 9.5pt; margin-top: 5pt;">Quick start: execution, pipes, redirection and history</p>
 </div>
 <div style="flex: 1; display: flex; flex-direction: column;">
  <img class="shot" src="${shot('job_control.png')}" style="width: 310pt; height: 194pt;"/>
  <p class="dim" style="font-size: 9.5pt; margin-top: 5pt;">Job control: Ctrl+Z stops, bg resumes, fg returns, Ctrl+C interrupts</p>
 </div>
</div>`, 'Demo'));

// ---- 12. Testing ----
slides.push(slide('Testing &amp; Verification', 'Unit tests, a pseudo terminal smoke test, and CI',
`<div class="col">
 <div class="panel" style="flex: 1.15;">
  <ul>
   <li><b>Parser unit tests</b>: 12 cases covering quotes, pipes, redirections and syntax errors</li>
   <li><b>Monitor unit tests</b>: /proc parsing plus a live PID</li>
   <li><b>pty smoke test</b>: Python drives a real pseudo terminal for Ctrl+C, Ctrl+Z, jobs, bg and fg</li>
   <li><b>Resource checks</b>: no zombies after reaping; fd count unchanged after 15 pipelines</li>
   <li><b>CI</b>: build and unit tests run on every push (GitHub Actions)</li>
  </ul>
 </div>
 <div style="flex: 1; display: flex; flex-direction: column; justify-content: center;">
  <img class="shot" src="${shot('tests.png')}" style="width: 305pt; height: 191pt;"/>
  <p class="dim" style="font-size: 9pt; margin-top: 5pt;">Local test run: CTest and the pty smoke suite</p>
 </div>
</div>`, 'Testing'));

// ---- 13. Limitations & future ----
slides.push(slide('Limitations &amp; Future Scope', 'Known ceilings, honest list',
`<div class="col">
 <div class="panel">
  <h2>Limitations</h2>
  <ul>
   <li>No globbing, variable expansion, <span class="mono">;</span> / <span class="mono">&amp;&amp;</span> / <span class="mono">||</span></li>
   <li>Builtins: no redirection, cannot join a pipeline</li>
   <li>No stderr redirection; no tab completion or line editing</li>
   <li>Background output can interleave with the prompt</li>
  </ul>
 </div>
 <div class="panel">
  <h2>Future scope</h2>
  <ul>
   <li>Line editing with GNU Readline (arrows, tab completion)</li>
   <li>Environment expansion, globbing, <span class="mono">2&gt;</span> stderr redirection</li>
   <li>Run builtins in subshells so they work inside pipelines</li>
   <li>Script files and a noninteractive mode with <span class="mono">; &amp;&amp; ||</span></li>
  </ul>
 </div>
</div>`, 'Limitations'));

// ---- 14. Thank you + git link ----
slides.push(`<!DOCTYPE html><html><head><style>${css}
.wrap { flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 0 60pt; }
h1 { font-size: 40pt; margin: 0; color: #edeff2; }
</style></head><body>
<div class="wrap">
  <div class="code" style="margin-bottom: 22pt;"><p class="dim">gunjan@minishell:~$ <span style="color:#b9e4a6">exit</span></p><p class="dim">goodbye</p></div>
  <h1>Thank You</h1>
  <p style="font-size: 13pt; color: #98acb5; margin-top: 10pt;">Source code, documentation, tests and CI are public in the repository:</p>
  <div class="code" style="margin-top: 8pt; padding: 14pt 22pt;">
    <p style="font-size: 13pt; color: #7ec98f; white-space: nowrap;">https://${GIT}</p>
  </div>
  <p style="font-size: 11pt; color: #5c6f7a; margin-top: 14pt;">[Student Name] &nbsp;&bull;&nbsp; [Class/Course] &nbsp;&bull;&nbsp; [College Name]</p>
</div>
</body></html>`);

// ---- write HTML files ----
slides.forEach((html, i) => fs.writeFileSync(path.join(OUT, `slide${String(i + 1).padStart(2, '0')}.html`), html));

(async () => {
  const pptx = new pptxgen();
  pptx.layout = 'LAYOUT_16x9';
  pptx.title = 'Mini Linux Shell with Process Manager';

  for (const f of fs.readdirSync(OUT).filter(f => f.endsWith('.html')).sort()) {
    await html2pptx(path.join(OUT, f), pptx);
    console.log('converted', f);
  }
  const outPath = path.join(__dirname, '..', 'minishell_presentation.pptx');
  await pptx.writeFile({ fileName: outPath });
  console.log('saved', outPath);

  // visual validation: screenshot each slide HTML at 720x405pt (960x540 px)
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 960, height: 540 } });
  const shots = [];
  for (const f of fs.readdirSync(OUT).filter(f => f.endsWith('.html')).sort()) {
    const shotPath = path.join(OUT, f.replace('.html', '.png'));
    await page.goto('file:///' + path.join(OUT, f).replace(/\\/g, '/'));
    await page.screenshot({ path: shotPath });
    shots.push(shotPath);
  }
  await browser.close();

  // montage into grids of 8 for quick review
  const sharp = require('sharp');
  for (let g = 0; g * 8 < shots.length; g++) {
    const batch = shots.slice(g * 8, g * 8 + 8);
    const cols = 4, w = 480, h = 270;
    const imgs = await Promise.all(batch.map(s => sharp(s).resize(w, h).png().toBuffer()));
    const rows = Math.ceil(batch.length / cols);
    await sharp({ create: { width: cols * w, height: rows * h, channels: 3, background: '#000' } })
      .composite(imgs.map((input, i) => ({ input, left: (i % cols) * w, top: Math.floor(i / cols) * h })))
      .jpeg({ quality: 90 }).toFile(path.join(OUT, `grid${g + 1}.jpg`));
  }
  console.log('validation grids written');
})().catch(e => { console.error(e); process.exit(1); });
