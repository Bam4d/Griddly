// Planning bot for Slime Ranch (slimefarm):
//   node studio/griddly-demo/tools/remaster/bot_slimefarm.mjs [episodes] [maxSteps] [level] [--record]
//
// Each step it reads getState() and pursues the level goals with a fixed priority:
//   1. carrying a gem and an order pedestal is empty -> walk into the nearest pedestal;
//   2. a gem lies on the ground and orders still need gems -> pick it up;
//   3. a throne still wants a King -> pick the colour with the most slime, feed two of its slimes up to
//      jellies (harvesting ripe crops for food), then push one jelly into the other (BFS over
//      (jelly, rancher) positions, every other object is an obstacle);
//   4. orders still need gems -> feed a King or a jelly (gems straight into the pocket), or grow a
//      slime into a jelly first.
// --record writes the first winning run of every level to apps/web/src/games/solutions/slimefarm.json
// (format 2, see replay_solutions.mjs).
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const pos = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const episodes = Number(pos[0] ?? 5);
const maxSteps = Number(pos[1] ?? 3000);
const onlyLevel = pos[2] !== undefined ? Number(pos[2]) : null;
const record = process.argv.includes("--record");
const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered/slimefarm.yaml"), "utf8");
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const MOVE = actions.indexOf("move"), FEED = actions.indexOf("feed");
const DIRS = [[1, -1, 0], [2, 0, -1], [3, 1, 0], [4, 0, 1]];
const GROUND = new Set(["grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge", "gate"]);
const SLIME = (n) => n.startsWith("slime_");

function snapshot() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const top = new Map(); // cell -> non-ground object
  let av = null;
  for (const o of s.objects) {
    if (o.location[0] < 0) continue;
    if (o.name === "rancher") av = o;
    if (GROUND.has(o.name)) continue;
    top.set(o.location[0] + o.location[1] * W, o);
  }
  const gv = s.globalVariables ?? g.getGlobalVariables();
  const val = (k) => { const v = gv[k]; return typeof v === "object" ? Number(Object.values(v)[0]) : Number(v); };
  return { W, H, top, av, objs: s.objects, gemsNeeded: val("gems_needed"), kingsNeeded: val("kings_needed") };
}

// BFS over walkable cells (empty/ground/gems). Returns the first action towards a cell from which
// `want(neighbourCell, obj)` holds, as [actionType, dir] (bumping/feeding in that direction when adjacent).
function goTo(st, want, kind = MOVE, avoid = new Set()) {
  const { W, H, top, av } = st;
  const start = av.location[0] + av.location[1] * W;
  const prev = new Map([[start, -1]]);
  const q = [start];
  for (let i = 0; i < q.length; i++) {
    const c = q[i];
    const x = c % W, y = (c - x) / W;
    for (const [a, dx, dy] of DIRS) {
      const nx = x + dx, ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
      const n = nx + ny * W;
      const o = top.get(n);
      if (o && want(n, o)) {
        if (c === start) return [kind, a];
        let cur = c;
        while (prev.get(cur) !== start) cur = prev.get(cur);
        const cx = cur % W, cy = (cur - cx) / W;
        return [MOVE, DIRS.find(([, ddx, ddy]) => av.location[0] + ddx === cx && av.location[1] + ddy === cy)[0]];
      }
      if (prev.has(n) || avoid.has(n)) continue;
      if (!o || o.name === "gem") { prev.set(n, c); q.push(n); }
    }
  }
  return null;
}

// Push jelly `a` into jelly `b`: BFS over (jelly, rancher). Returns the first move or null.
function pushPlan(st, a, b) {
  const { W, H, top, av } = st;
  const ia = a.location[0] + a.location[1] * W, ib = b.location[0] + b.location[1] * W;
  const walk = (c) => { const o = top.get(c); return c === ia || !o || o.name === "gem"; };
  const boxOk = (c) => { const o = top.get(c); return c === ia || !o || o.name === "gem"; };
  const start = ia * W * H + av.location[0] + av.location[1] * W;
  const prev = new Map([[start, [-1, 0]]]);
  const q = [start];
  for (let i = 0; i < q.length; i++) {
    const s = q[i];
    const box = Math.floor(s / (W * H)), p = s % (W * H);
    const px = p % W, py = (p - px) / W;
    for (const [d, dx, dy] of DIRS) {
      const nx = px + dx, ny = py + dy;
      if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
      const n = nx + ny * W;
      let ns;
      if (n === box) {
        const bx = nx + dx, by = ny + dy;
        if (bx < 0 || by < 0 || bx >= W || by >= H) continue;
        const nb = bx + by * W;
        if (nb === ib) { // success: unwind
          let cur = s, first = d;
          while (prev.get(cur)[0] !== -1) { first = prev.get(cur)[1]; cur = prev.get(cur)[0]; }
          return [MOVE, first];
        }
        if (!boxOk(nb) || nb === ib) continue;
        ns = nb * W * H + n;
      } else {
        if (!walk(n) || n === ib) continue;
        ns = box * W * H + n;
      }
      if (prev.has(ns)) continue;
      prev.set(ns, [s, d]);
      q.push(ns);
    }
  }
  return null;
}

let stuck = 0;
function decide() {
  const st = snapshot();
  const { av, top } = st;
  if (!av) return null;
  const food = av.variables.food, gems = av.variables.gems;
  const all = [...top.values()];
  const slimes = all.filter((o) => SLIME(o.name));
  const harvest = () => (food < 3 ? goTo(st, (n, o) => o.name === "crop" && o.variables.ripe === 1) : null);
  const feedSlime = (pred) => goTo(st, (n, o) => (SLIME(o.name) || o.name === "king") && pred(o), FEED);
  // 1. fill orders
  if (gems > 0 && st.gemsNeeded > 0) {
    const r = goTo(st, (n, o) => o.name === "pedestal");
    if (r) return r;
  }
  // 2. pick up gems lying around
  if (st.gemsNeeded > gems) {
    const r = goTo(st, (n, o) => false || (o.name === "gem"), MOVE);
    if (r) return r;
  }
  // 3. Kings
  if (st.kingsNeeded > 0) {
    const byColour = {};
    for (const s of slimes) (byColour[s.name] ??= []).push(s);
    const cols = Object.entries(byColour).filter(([, v]) => v.length >= 2)
      .sort((x, y) => y[1].reduce((t, s) => t + (1 << s.variables.tier), 0) - x[1].reduce((t, s) => t + (1 << s.variables.tier), 0));
    for (const [name, list] of cols) {
      const jellies = list.filter((s) => s.variables.tier === 2);
      if (jellies.length >= 2) {
        let best = null;
        for (const a of jellies) for (const b of jellies) if (a !== b) {
          const r = pushPlan(st, a, b);
          if (r) { best = r; break; }
        }
        if (best) return best;
        // cornered jellies: grow a third one of this colour, else try the next colour
        if (jellies.length > list.filter((x) => x.variables.tier === 2).length - 1 && list.length === jellies.length) continue;
      }
      // grow the highest non-jelly slime of this colour
      if (food > 0) {
        const grow = list.filter((s) => s.variables.tier < 2).sort((x, y) => y.variables.tier - x.variables.tier);
        if (grow.length) {
          const target = new Set(grow.slice(0, Math.max(1, 2 - jellies.length)).map((s) => s.location.join()));
          const r = feedSlime((o) => o.name === name && target.has(o.location.join()));
          if (r) return r;
        }
      } else {
        const r = harvest();
        if (r) return r;
      }
      break;
    }
  }
  // 4. gems for orders
  if (st.gemsNeeded > gems) {
    const producers = all.filter((o) => o.name === "king" || (SLIME(o.name) && o.variables.tier === 2));
    if (food > 0) {
      const r = producers.length ? feedSlime((o) => o.name === "king" || o.variables.tier === 2)
        : feedSlime((o) => SLIME(o.name) && o.variables.tier < 2);
      if (r) return r;
    } else {
      const r = harvest();
      if (r) return r;
    }
  }
  stuck++;
  return [MOVE, stuck % 7 === 0 ? 1 + (stuck % 4) : 0];
}

const recorded = [];
const n = g.getLevelCount();
let total = 0, totalWins = 0;
for (let l = 0; l < n; l++) {
  if (onlyLevel !== null && l !== onlyLevel) continue;
  let wins = 0, losses = 0, timeouts = 0;
  const steps = [];
  for (let ep = 0; ep < episodes; ep++) {
    g.loadLevel(l);
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
    if (res === "timeout" && process.env.DEBUG) {
      const st = snapshot();
      console.log(`    ep${ep} timeout: rancher ${st.av?.location} ${JSON.stringify(st.av?.variables)} gems_needed ${st.gemsNeeded} kings_needed ${st.kingsNeeded}`);
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
console.log(`slimefarm: bot won ${totalWins}/${total}`);
if (record) {
  const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions/slimefarm.json");
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
