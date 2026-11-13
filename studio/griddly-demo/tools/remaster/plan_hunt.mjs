// Best-first planner over the exact engine state, for levels with A* hunters (and anything else the
// cautious bot can't record):
//   node studio/griddly-demo/tools/remaster/plan_hunt.mjs <game_id> <level> [--seed=N] [--budget=nodes] [--record]
//
// The engine is deterministic for a given seed, so a node is just an action prefix: to expand it we
// reset + replay the prefix (cheap: ~50us/step), read getState() once and generate macro-actions:
//   * a shortest path (cut to <= 10 moves) towards each of the nearest few goal objects,
//   * "kite" paths to the reachable cells furthest (in hunter steps) from every hunter,
//   * waiting 2 ticks, and (zelda) sword strikes at enemies within 2 tiles in line.
// Every macro is simulated in the engine from the node's prefix; macros that end the run with a loss are
// dropped, the rest become children scored by (reward gained, hunter distance, length). The best-first
// order makes it greedy with backtracking: when every continuation dies it falls back to an older node.
// With --record the winning run replaces this level's entry in apps/web/src/games/solutions/<id>.json
// ({format: 2, levels: [{seed, actions: [[actionTypeIdx, actionId]...]}]}).
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const GROUND = ["grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge"];
const GAMES = {
  zelda: { walk: [...GROUND], enemies: ["spider", "bat", "hunter"], hunters: ["hunter"],
    goals: (a) => ["key", ...(a.variables.keys > 0 ? ["door", "exit"] : [])], attack: true },
  butterflies: { walk: GROUND, enemies: ["spider", "hunter"], hunters: ["hunter"], goals: () => ["butterfly"] },
  spider_nest: { walk: GROUND, enemies: ["spider", "hunter"], hunters: ["hunter"], goals: () => ["gem"] },
  spiders: { walk: GROUND, enemies: ["spider", "hunter"], hunters: ["hunter"],
    goals: (a) => ["gem", "key", ...(a.variables.keys > 0 ? ["door"] : [])] },
};

const id = process.argv[2];
const level = Number(process.argv[3]);
const opt = (k, d) => Number((process.argv.find((a) => a.startsWith(`--${k}=`)) ?? `=${d}`).split("=")[1]);
const seed = opt("seed", 1);
const budget = opt("budget", 3000);
const maxLen = opt("maxlen", 3000);
const record = process.argv.includes("--record");
const W_GOAL = opt("wgoal", 3), W_SAFE = opt("wsafe", 2);
const cfg = GAMES[id];
const gdyFile = id === "spiders" ? "resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml"
  : `resources/games/Single-Player/Remastered/${id}.yaml`;
const gdy = fs.readFileSync(path.join(root, gdyFile), "utf8");
const avatarName = gdy.match(/AvatarObject:\s*(\S+)/)[1];
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const act = (name, a) => (actions.length > 1 ? [actions.indexOf(name), a] : [a]);
const DIRS = [[1, -1, 0], [2, 0, -1], [3, 1, 0], [4, 0, 1]];
const walk = new Set(cfg.walk);
const killers = new Set(cfg.enemies);
const hunterSet = new Set(cfg.hunters);
g.loadLevel(level);

// replay a prefix; returns {dead, won, reward}
function replay(prefix, from = 0, base = null) {
  if (!base) { g.seed(seed); g.reset(); }
  let reward = 0;
  for (let i = from; i < prefix.length; i++) {
    const r = g.step(prefix[i]);
    reward += Number(r?.reward ?? r?.rewards?.[0] ?? 0) || 0;
    if (r?.terminated) return { dead: r.info?.playerResults?.["1"] !== "Win", won: r.info?.playerResults?.["1"] === "Win", reward, at: i };
  }
  return { dead: false, won: false, reward };
}

function analyse() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const cells = new Map();
  let av = null;
  const hunters = [], foes = [];
  for (const o of s.objects) {
    if (o.location[0] < 0) continue;
    if (o.name === avatarName) { av = o; continue; }
    const k = o.location[0] + o.location[1] * W;
    if (!cells.has(k)) cells.set(k, []);
    cells.get(k).push(o.name);
    if (hunterSet.has(o.name)) hunters.push(o);
    if (killers.has(o.name)) foes.push(k);
  }
  return { W, H, cells, av, hunters, foes };
}

function bfsFrom(W, H, start, passable) {
  const d = new Int32Array(W * H).fill(1e6);
  const prev = new Int32Array(W * H).fill(-1);
  d[start] = 0;
  const q = [start];
  for (let i = 0; i < q.length; i++) {
    const c = q[i], x = c % W, y = (c - x) / W;
    for (const [, dx, dy] of DIRS) {
      const nx = x + dx, ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
      const n = nx + ny * W;
      if (d[n] < 1e6 || !passable(n)) continue;
      d[n] = d[c] + 1;
      prev[n] = c;
      q.push(n);
    }
  }
  return { d, prev };
}

function pathTo(W, prev, start, target) {
  const cellsOnPath = [];
  for (let c = target; c !== start && c >= 0; c = prev[c]) cellsOnPath.push(c);
  cellsOnPath.reverse();
  const out = [];
  let cur = start;
  for (const c of cellsOnPath) {
    const dx = (c % W) - (cur % W), dy = Math.floor(c / W) - Math.floor(cur / W);
    out.push(DIRS.find(([, a, b]) => a === dx && b === dy)[0]);
    cur = c;
  }
  return out;
}

function candidates(st) {
  const { W, H, cells, av, hunters, foes } = st;
  const start = av.location[0] + av.location[1] * W;
  const objs = (k) => cells.get(k) ?? [];
  const free = (k) => objs(k).every((o) => walk.has(o));
  const near = new Set();
  for (const f of foes) {
    near.add(f);
    for (const [, dx, dy] of DIRS) near.add(f + dx + dy * W);
  }
  const goals = new Set(cfg.goals(av));
  const out = [];
  let goalDist = 1e3;
  // hunter distance maps
  const hd = hunters.map((h) => ({
    d: bfsFrom(W, H, h.location[0] + h.location[1] * W, (n) => objs(n).every((o) => GROUND.includes(o)) || n === start).d,
    pace: Math.max(1, h.variables?.pace || 3),
  }));
  const hmin = (n) => hd.reduce((m, { d, pace }) => Math.min(m, d[n] * pace), 1e6);
  // goal paths: BFS treating goal cells as terminal
  for (const avoidNear of [true, false]) {
    const { d, prev } = bfsFrom(W, H, start, (n) => free(n) && !(avoidNear && near.has(n) && n !== start));
    const targets = [];
    for (const [k, list] of cells) {
      if (!list.some((o) => goals.has(o))) continue;
      const x = k % W, y = (k - x) / W;
      let best = -1, bd = 1e6;
      for (const [, dx, dy] of DIRS) {
        const n = x + dx + (y + dy) * W;
        if (x + dx < 0 || y + dy < 0 || x + dx >= W || y + dy >= H) continue;
        if (d[n] < bd) { bd = d[n]; best = n; }
      }
      if (best >= 0 && bd < 1e6) targets.push({ k, via: best, len: bd });
    }
    targets.sort((a, b) => a.len - b.len);
    if (!avoidNear && targets.length) goalDist = targets[0].len;
    for (const t of targets.slice(0, 4)) {
      const p = pathTo(W, prev, start, t.via);
      const dx = (t.k % W) - (t.via % W), dy = Math.floor(t.k / W) - Math.floor(t.via / W);
      p.push(DIRS.find(([, a, b]) => a === dx && b === dy)[0]);
      out.push(p.slice(0, 12).map((a) => act("move", a)));
      if (p.length > 6) out.push(p.slice(0, 5).map((a) => act("move", a)));
    }
  }
  // time-aware goal paths: every cell entered at move k must be > ceil(k / pace) + margin hunter steps
  // from every hunter (the bot's safety model), so these routes slip past the hunters
  if (hd.length) {
    for (const margin of [0, 1, 2]) {
      const safe = (n, k) => hd.every(({ d, pace }) => d[n] > Math.ceil(k / pace) + margin);
      const prev = new Map([[start, -1]]), depth = new Map([[start, 0]]);
      const q = [start];
      let found = null;
      for (let i = 0; i < q.length && !found; i++) {
        const c = q[i], x = c % W, y = (c - x) / W, k = depth.get(c) + 1;
        for (const [a, dx, dy] of DIRS) {
          const nx = x + dx, ny = y + dy;
          if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
          const n = nx + ny * W;
          if (prev.has(n) || near.has(n) && !hunters.some((h) => h.location[0] + h.location[1] * W === n) && foes.includes(n)) continue;
          if (objs(n).some((o) => goals.has(o))) { if (safe(c, k)) { found = [c, a]; break; } continue; }
          if (!free(n) || !safe(n, k)) continue;
          prev.set(n, c); depth.set(n, k); q.push(n);
        }
      }
      if (found) {
        const p = [];
        for (let c = found[0]; c !== start; c = prev.get(c)) {
          const pc = prev.get(c);
          p.push(DIRS.find(([, a, b]) => a === (c % W) - (pc % W) && b === Math.floor(c / W) - Math.floor(pc / W))[0]);
        }
        p.reverse();
        p.push(found[1]);
        out.push(p.slice(0, 16).map((a) => act("move", a)));
      }
    }
  }
  // kite: reachable cells within 10 steps that are furthest from the hunters
  if (hd.length) {
    const { d, prev } = bfsFrom(W, H, start, (n) => free(n) && !near.has(n));
    const spots = [];
    for (let n = 0; n < W * H; n++) if (d[n] > 0 && d[n] <= 16) spots.push([n, hmin(n) - d[n] * 0.5]);
    spots.sort((a, b) => b[1] - a[1]);
    for (const [n] of spots.slice(0, 4)) {
      const p = pathTo(W, prev, start, n);
      out.push(p.slice(0, 6).map((a) => act("move", a)));
      if (p.length > 6) out.push(p.map((a) => act("move", a)));
    }
  }
  out.push([act("move", 0), act("move", 0)]);
  // zelda: strike at enemies within 2 tiles in line
  if (cfg.attack) {
    for (const [a, dx, dy] of DIRS) {
      const x = av.location[0], y = av.location[1];
      for (const r of [1, 2]) {
        const n = x + dx * r + (y + dy * r) * W;
        if (x + dx * r < 0 || y + dy * r < 0 || x + dx * r >= W || y + dy * r >= H) break;
        if (objs(n).some((o) => killers.has(o))) { out.push([act("attack", a)]); break; }
        if (!free(n)) break;
      }
    }
  }
  return { out, hmin: hd.length ? hmin(start) : 99, goalDist };
}

// --------------------------------------------------------------------------------------------- search
const t0 = Date.now();
let heap = [{ prefix: [], reward: 0, score: 0 }];
const seen = new Set();
let expanded = 0, sims = 0, won = null, bestReward = -1e9;
const push = (n) => {
  heap.push(n);
};
while (heap.length && expanded < budget) {
  heap.sort((a, b) => a.score - b.score);
  const node = heap.shift();
  expanded++;
  replay(node.prefix);
  const st = analyse();
  if (!st.av) continue;
  const key = `${st.av.location}|${node.reward}|${st.hunters.map((h) => h.location).join(";")}|${Math.floor(node.prefix.length / 4)}`;
  if (seen.has(key)) continue;
  seen.add(key);
  const { out } = candidates(st);
  for (const macro of out) {
    sims++;
    const r = replay(macro, 0, true); // continue from the node's state
    // continue-from-state consumed the engine; restore before the next candidate
    const full = node.prefix.concat(r.at !== undefined ? macro.slice(0, r.at + 1) : macro);
    if (r.won) { won = full; break; }
    if (!r.dead && full.length < maxLen) {
      const after = analyse();
      if (!after.av) { replay(node.prefix); continue; }
      const { hmin: hm, goalDist } = candidates(after);
      const reward = node.reward + r.reward;
      push({ prefix: full, reward, score: -reward * 1000 + goalDist * W_GOAL - Math.min(hm, 9) * W_SAFE + full.length * 0.1 });
      bestReward = Math.max(bestReward, reward);
    }
    replay(node.prefix);
  }
  if (won) break;
  if (expanded % 100 === 0) console.log(`  expanded ${expanded} sims ${sims} open ${heap.length} best reward ${bestReward} (${((Date.now() - t0) / 1000).toFixed(0)}s)`);
  if (heap.length > 4000) heap = heap.sort((a, b) => a.score - b.score).slice(0, 2000);
}
if (!won) {
  const best = heap.sort((a, b) => a.score - b.score)[0] ?? null;
  if (best && process.env.DEBUG) {
    for (const back of [12, 0]) {
      replay(best.prefix.slice(0, Math.max(0, best.prefix.length - back)));
      const st = analyse();
      const grid = Array.from({ length: st.H }, () => Array(st.W).fill("."));
      for (const [k, list] of st.cells) {
        const o = list.find((x) => !GROUND.includes(x));
        if (o) grid[Math.floor(k / st.W)][k % st.W] = { wall: "W", hunter: "H", spider: "S", gem: "g", lava: "L", brood: "B", nest: "N" }[o] ?? "#";
      }
      if (st.av) grid[st.av.location[1]][st.av.location[0]] = "A";
      console.log(`best node: len ${best.prefix.length - back} reward ${best.reward}\n` + grid.map((r) => r.join("")).join("\n"));
      if (back === 0) {
        for (const macro of candidates(st).out) {
          const r = replay(macro, 0, true);
          console.log(JSON.stringify(macro.map((m) => m[m.length - 1])), r.dead ? `DEAD at ${r.at}` : r.won ? "WON" : `ok +${r.reward}`);
          replay(best.prefix);
        }
      }
    }
  }
  console.log(`${id} L${level} seed ${seed}: no win after ${expanded} expansions (best reward ${bestReward})`);
  process.exit(1);
}
// verify
const v = replay(won);
console.log(`${id} L${level} seed ${seed}: WIN in ${won.length} actions (verified ${v.won}) after ${expanded} expansions, ${sims} sims, ${((Date.now() - t0) / 1000).toFixed(0)}s`);
if (record && v.won) {
  // --out=<file>: write there instead (parallel per-level recording, merged afterwards)
  const outArg = process.argv.find((a) => a.startsWith("--out="));
  const out = outArg ? outArg.slice(6) : path.join(here, "../../apps/web/src/games/solutions", `${id}.json`);
  const old = fs.existsSync(out) ? JSON.parse(fs.readFileSync(out, "utf8")) : { format: 2, levels: [] };
  const n = g.getLevelCount();
  old.levels[level] = { seed, actions: won.map((x) => (x.length === 1 ? [0, x[0]] : x)) };
  for (let i = 0; i < n; i++) old.levels[i] ??= null;
  old.levels.length = n;
  fs.writeFileSync(out, JSON.stringify(old));
  console.log(`record: wrote level ${level} to ${out}`);
}
