// Render captured minishell sessions as clean terminal screenshots for the report and slides.
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const HERE = __dirname;
const SHOTS = path.join(HERE, '..', '..', 'docs', 'screenshots');
fs.mkdirSync(SHOTS, { recursive: true });

const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

const CIRC = '<span class="dot red"></span><span class="dot yellow"></span><span class="dot green"></span>';

function prompt(text) {
  const at = text.indexOf('$ ');
  if (at === -1) return `<span class="user">${esc(text)}</span>`;
  return `<span class="user">${esc(text.slice(0, at))}$</span> <span class="cmd">${esc(text.slice(at + 2))}</span>`;
}

function lineHtml(line) {
  if (line.startsWith('^C') || line.startsWith('^Z')) {
    return `<span class="key">${esc(line.slice(0, 2))}</span>${line.length > 2 ? ' ' + prompt(line.slice(2)) : ''}`;
  }
  if (line.startsWith('gunjan@minishell:')) return prompt(line);
  if (/^\[[0-9]+\] (Started|Running|Stopped|Done)/.test(line)) return `<span class="job">${esc(line)}</span>`;
  if (line.startsWith('====')) return `<span class="head">${esc(line)}</span>`;
  if (line.startsWith('minishell:')) return `<span class="err">${esc(line)}</span>`;
  if (line.startsWith('sent signal')) return `<span class="job">${esc(line)}</span>`;
  if (/PASS|Passed|passed|tests passed/.test(line)) return `<span class="ok">${esc(line)}</span>`;
  if (/^PID\s+PPID|^-{5,}$/.test(line)) return `<span class="dim">${esc(line)}</span>`;
  if (/^Test project|^\s+Start|^Internal ctest|^Total Test|^[0-9]+\/[0-9]+ Test/.test(line)) {
    return `<span class="dim">${esc(line)}</span>`;
  }
  if (/^(no active jobs)$/.test(line)) return `<span class="dim">${esc(line)}</span>`;
  return `<span class="out">${esc(line)}</span>`;
}

function htmlFor(s) {
  const lines = s.text.replace(/\t/g, '    ').replace(/\n+$/, '').split('\n');
  const body = lines.map((l) => `<div class="line">${l ? lineHtml(l) : '&nbsp;'}</div>`).join('\n');
  return `<!DOCTYPE html><html><head><meta charset="utf-8"><style>
  * { box-sizing: border-box; }
  html, body { margin: 0; width: 1600px; height: 1000px; background: #080b10; }
  body { padding: 18px; font-family: 'Cascadia Mono', 'Consolas', 'Courier New', monospace; }
  .win { width: 1564px; height: 964px; background: #0d1117; border: 1px solid #2b3542;
         border-radius: 14px; box-shadow: 0 18px 50px rgba(0,0,0,0.55); overflow: hidden;
         display: flex; flex-direction: column; }
  .bar { height: 54px; background: linear-gradient(#212a35, #1a222c); border-bottom: 1px solid #2b3542;
         display: flex; align-items: center; padding: 0 18px; gap: 8px; flex: 0 0 54px; }
  .dot { width: 12px; height: 12px; border-radius: 50%; display: inline-block; }
  .red { background: #ff5f57; } .yellow { background: #febc2e; } .green { background: #28c840; }
  .title { color: #9aa7b4; font-size: 15px; margin-left: 12px; letter-spacing: 0.2px; }
  .right { margin-left: auto; color: #5d6b78; font-size: 13px; }
  .term { flex: 1; padding: 20px 26px; overflow: hidden; }
  .line { font-size: 26px; line-height: 38px; white-space: pre; color: #c9d1d9; }
  .user { color: #7ee787; font-weight: 600; }
  .cmd { color: #e6edf3; font-weight: 600; }
  .out { color: #c9d1d9; }
  .dim { color: #6e7d8a; }
  .job { color: #e3b341; }
  .head { color: #79c0ff; font-weight: 600; }
  .err { color: #ff7b72; }
  .ok { color: #7ee787; font-weight: 600; }
  .key { color: #e3b341; font-weight: 700; }
  </style></head><body>
  <div class="win">
    <div class="bar">${CIRC}<span class="title">${esc(s.title)}</span><span class="right">minishell  &bull;  C++17  &bull;  POSIX</span></div>
    <div class="term">${body}</div>
  </div>
  </body></html>`;
}

(async () => {
  const sessions = JSON.parse(fs.readFileSync(path.join(HERE, 'transcripts.json'), 'utf8'));
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1600, height: 1000 }, deviceScaleFactor: 2 });
  for (const s of sessions) {
    await page.setContent(htmlFor(s), { waitUntil: 'load' });
    const out = path.join(SHOTS, `${s.name}.png`);
    await page.screenshot({ path: out });
    console.log('saved', out);
  }
  await browser.close();
})().catch((e) => { console.error(e); process.exit(1); });
