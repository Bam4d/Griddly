// Cautious shooter bot for Bufo vs the Benchmaxxers (benchmaxx):
//   node studio/griddly-demo/tools/remaster/bot_benchmaxx.mjs [episodes] [maxSteps] [level] [--record]
//
// Every step it reads getState() and, in priority order:
//   1. throws mayo at a bot standing in a straight line with a clear line of fire within 7 tiles
//      (an adjacent bot first), when the throw is ready;
//   2. dodges: if a bot is adjacent (or two cells away), steps to the free neighbour that maximises the
//      distance to the nearest bot;
//   3. throws at a trophy in line of fire (any distance) when ready;
//   4. walks (BFS, avoiding bots and the cells next to them) to the nearest firing spot: a cell with a
//      clear line to a trophy, 2..8 tiles away; GPUs / jars (when hurt) within 6 steps come first.
// --record writes the first winning run per level to apps/web/src/games/solutions/benchmaxx.json
// (format 2, replayed by replay_solutions.mjs).
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const pos = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const episodes = Number(pos[0] ?? 10);
const maxSteps = Number(pos[1] ?? 2500);
const onlyLevel = pos[2] !== undefined ? Number(pos[2]) : null;
const record = process.argv.includes("--record");
const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered/benchmaxx.yaml"), "utf8");
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const MOVE = actions.indexOf("move"), THROW = actions.indexOf("throw");
const DIRS = [[1, -1, 0], [2, 0, -1], [3, 1, 0], [4, 0, 1]];
const PASS = new Set(["cable", "puddle", "gpu", "jar", "mayo", "grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge"]);
const WALKABLE = new Set([...PASS].filter((n) => n !== "jar")); // jar handled by hp
const BOTS = new Set(["hunter", "wanderer"]);
const TROPHY = new Set(["trophy", "trophy_gold", "trophy_sota"]);

function decide() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const cells = new Map();
  let av = null;
  const bots = [], trophies = [], items = [];
  for (const o of s.objects) {
    if (o.location[0] < 0) continue;
    if (o.name === "bufo") { av = o; continue; }
    const k = o.location[0] + o.location[1] * W;
    if (!cells.has(k)) cells.set(k, []);
    cells.get(k).push(o.name);
    if (BOTS.has(o.name)) bots.push(o.location);
    if (TROPHY.has(o.name)) trophies.push(o.location);
    if (o.name === "gpu" || o.name === "jar") items.push([o.name, o.location]);
  }
  if (!av) return null;
  const [ax, ay] = av.location;
  const hp = av.variables.hp, ready = av.variables.ready === 1;
  const inb = (x, y) => x >= 0 && y >= 0 && x < W && y < H;
  const objs = (x, y) => cells.get(x + y * W) ?? [];
  const passable = (x, y) => inb(x, y) && objs(x, y).every((o) => PASS.has(o));
  const walkable = (x, y) => inb(x, y) && objs(x, y).every((o) => WALKABLE.has(o) || (o === "jar" && hp < 3));
  // first non-passable thing along a ray (what a mayo thrown that way hits)
  const ray = (x, y, dx, dy, max = 99) => {
    for (let i = 1; i <= max; i++) {
      const nx = x + dx * i, ny = y + dy * i;
      if (!inb(nx, ny)) return null;
      const o = objs(nx, ny).find((n) => !PASS.has(n));
      if (o) return [o, i];
    }
    return null;
  };
  const dist = (p, q) => Math.abs(p[0] - q[0]) + Math.abs(p[1] - q[1]);
  const nearestBot = (x, y) => Math.min(99, ...bots.map((b) => dist(b, [x, y])));
  // 1. shoot bots in line
  if (ready) {
    let best = null;
    for (const [a, dx, dy] of DIRS) {
      const h = ray(ax, ay, dx, dy, 7);
      if (h && BOTS.has(h[0]) && (!best || h[1] < best[1])) best = [a, h[1]];
    }
    if (best) return [THROW, best[0]];
  }
  // 2. dodge
  const near = nearestBot(ax, ay);
  if (near <= 2) {
    let best = 0, bestD = near;
    for (const [a, dx, dy] of DIRS) {
      const nx = ax + dx, ny = ay + dy;
      if (!walkable(nx, ny)) continue;
      const d = nearestBot(nx, ny);
      if (d > bestD) { bestD = d; best = a; }
    }
    if (best) return [MOVE, best];
  }
  // 3. shoot trophies in line
  if (ready) {
    for (const [a, dx, dy] of DIRS) {
      const h = ray(ax, ay, dx, dy);
      if (h && TROPHY.has(h[0])) return [THROW, a];
    }
  }
  // 4. go to a firing spot / item
  const danger = new Set();
  for (const [bx, by] of bots) {
    danger.add(bx + by * W);
    for (const [, dx, dy] of DIRS) danger.add(bx + dx + (by + dy) * W);
  }
  const goal = (x, y) => {
    for (const [name, [ix, iy]] of items) if (ix === x && iy === y && (name === "gpu" || hp < 3) && dist([ax, ay], [x, y]) <= 6) return true;
    for (const [, dx, dy] of DIRS) {
      const h = ray(x, y, dx, dy, 8);
      if (h && TROPHY.has(h[0]) && h[1] >= 2) return true;
    }
    return false;
  };
  const bfs = (avoid) => {
    const start = ax + ay * W;
    const prev = new Map([[start, [-1, 0]]]);
    const q = [start];
    for (let i = 0; i < q.length; i++) {
      const c = q[i];
      const x = c % W, y = (c - x) / W;
      if (c !== start && goal(x, y)) {
        let cur = c;
        while (prev.get(cur)[0] !== start) cur = prev.get(cur)[0];
        return prev.get(cur)[1];
      }
      for (const [a, dx, dy] of DIRS) {
        const nx = x + dx, ny = y + dy, n = nx + ny * W;
        if (prev.has(n) || !walkable(nx, ny) || avoid.has(n)) continue;
        prev.set(n, [c, a]);
        q.push(n);
      }
    }
    return 0;
  };
  if (goal(ax, ay) && !ready) return [MOVE, 0];
  let step = bfs(danger);
  if (!step) step = bfs(new Set(bots.map(([x, y]) => x + y * W)));
  if (step) return [MOVE, step];
  return [MOVE, 0];
}

const recorded = [];
const n = g.getLevelCount();
let total = 0, totalWins = 0;
for (let l = 0; l < n; l++) {
  if (onlyLevel !== null && l !== onlyLevel) continue;
  g.loadLevel(l);
  let wins = 0, losses = 0, timeouts = 0;
  const steps = [];
  for (let ep = 0; ep < episodes; ep++) {
    g.seed(ep + 1);
    g.reset();
    let res = "timeout";
    const taken = [];
    for (let t = 0; t < maxSteps; t++) {
      const d = decide();
      if (!d) { res = "lose"; break; }
      taken.push(d);
      const r = g.step(d);
      if (r && r.terminated) {
        res = r.info?.playerResults?.["1"] === "Win" ? "win" : "lose";
        steps.push(t + 1);
        break;
      }
    }
    if (process.env.DEBUG && res !== "win") {
      const st = g.getState();
      const cnt = {};
      for (const o of st.objects) cnt[o.name] = (cnt[o.name] ?? 0) + 1;
      console.log(`    ep${ep} ${res}`, JSON.stringify(g.getGlobalVariables()), JSON.stringify(cnt));
    }
    if (res === "win" && record && !recorded[l]) recorded[l] = { seed: ep + 1, actions: taken };
    if (res === "win") wins++;
    else if (res === "lose") losses++;
    else timeouts++;
  }
  total += episodes;
  totalWins += wins;
  console.log(`  L${l}: ${g.width}x${g.height} bot win ${wins} lose ${losses} timeout ${timeouts} / ${episodes}  steps ${steps.join(",")}`);
}
console.log(`benchmaxx: bot won ${totalWins}/${total}`);

if (record) {
  const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions/benchmaxx.json");
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
