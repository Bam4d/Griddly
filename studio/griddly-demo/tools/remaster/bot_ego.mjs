// Full-state planning bot for the egocentric (relative-control) remasters: torchlight, nightgarden.
//   node studio/griddly-demo/tools/remaster/bot_ego.mjs <game_id> [episodes] [maxSteps] [level] [--record]
//
// The bot reads getState() (the whole map, not the egocentric view) every tick and plans over
// (x, y, facing) states with the avatar's real action set: 1 turn left, 2 forward, 3 turn right,
// 4 back up (facing kept), plus `wait`. It reports per-level win rates using the GDY's own termination.
// With --record, the first winning episode per level is written to
// apps/web/src/games/solutions/<id>.json as {format: 2, levels: [{seed, actions: [[actionTypeIdx, id]]}]}.
//
// Game logic per id:
//   torchlight: go for the nearest crystal whose round trip (to it, then on to the nearest torch/brazier)
//     fits in the remaining fuel, otherwise go and refuel. Cells next to an unfrozen spider are avoided,
//     and an adjacent spider is faced (which freezes it) or backed away from.
//   nightgarden: guards are deterministic; the bot simulates them forward from the current state and
//     plans in space-time (x, y, facing, t), never standing in a guard's sight line (unless in tall
//     grass). It takes the nearest lantern, then the gate.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const GROUND = ["grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge"];
const VEC = { UP: [0, -1], RIGHT: [1, 0], DOWN: [0, 1], LEFT: [-1, 0] };
const DV = [[0, -1], [1, 0], [0, 1], [-1, 0]]; // facing index 0 up, 1 right, 2 down, 3 left
const fIdx = (o) => DV.findIndex(([x, y]) => x === VEC[o][0] && y === VEC[o][1]);

const id = process.argv[2];
const posArgs = process.argv.slice(2).filter((a) => !a.startsWith("-"));
const episodes = Number(posArgs[1] ?? 5);
const maxSteps = Number(posArgs[2] ?? 1500);
const onlyLevel = posArgs[3] !== undefined ? Number(posArgs[3]) : null;
const record = process.argv.includes("--record");
const verbose = process.argv.includes("-v");
const DEBUG = process.argv.includes("-vv");
const EPS = Number(process.env.EPS ?? 0.03);
// tiny seeded PRNG so recorded runs are reproducible
let rngState = 1;
const rand = () => ((rngState = (rngState * 1103515245 + 12345) >>> 0) / 4294967296);

const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered", `${id}.yaml`), "utf8");
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const MOVE = actions.indexOf("move"), WAIT = actions.indexOf("wait");

// ------------------------------------------------------------------------------------------------ world
function world() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const cells = new Map();
  let av = null;
  const by = {};
  for (const o of s.objects) {
    if (o.location[0] < 0) continue;
    if (o.name === "avatar") av = o;
    (by[o.name] ??= []).push(o);
    const k = o.location[0] + o.location[1] * W;
    if (!cells.has(k)) cells.set(k, []);
    cells.get(k).push(o.name);
  }
  const gv = g.getGlobalVariables();
  const glob = Object.fromEntries(Object.entries(gv).map(([k, v]) => [k, v[0] ?? v["0"]]));
  return { W, H, cells, av, by, glob, objs: s.objects };
}

// BFS over (x, y, f). passable(k) -> bool for entering a cell. Returns dist + first action per state.
function plan(w, passable, start, stateOk = () => true) {
  const { W, H } = w;
  const N = W * H * 4;
  const dist = new Int32Array(N).fill(-1);
  const first = new Int8Array(N);
  const s0 = (start.x + start.y * W) * 4 + start.f;
  dist[s0] = 0;
  const q = [s0];
  for (let qi = 0; qi < q.length; qi++) {
    const s = q[qi];
    const f = s & 3, k = s >> 2, x = k % W, y = (k - x) / W;
    const nexts = [];
    nexts.push([1, (k << 2) | ((f + 3) & 3)]);
    nexts.push([3, (k << 2) | ((f + 1) & 3)]);
    for (const [a, sgn] of [[2, 1], [4, -1]]) {
      const nx = x + sgn * DV[f][0], ny = y + sgn * DV[f][1];
      if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
      const nk = nx + ny * W;
      if (passable(nk, a === 2, f)) nexts.push([a, (nk << 2) | f]);
    }
    for (const [a, n] of nexts) {
      if (dist[n] >= 0 || !stateOk(n)) continue;
      dist[n] = dist[s] + 1;
      first[n] = s === s0 ? a : first[s];
      q.push(n);
    }
  }
  return { dist, first };
}

// Reverse distances: R[s] = min actions from state s until a "goal action" is performed (e.g. bumping a
// torch). goalStates: Map state -> cost of the final action (1).
function reverse(w, passable, goalStates, stateOk = () => true) {
  const { W, H } = w;
  const N = W * H * 4;
  const R = new Int32Array(N).fill(-1);
  const q = [];
  for (const [s, c] of goalStates) { R[s] = c; q.push(s); }
  // predecessors of state s=(k,f): turns from (k, f±1); forward from (k - d_f, f); back from (k + d_f, f)
  for (let qi = 0; qi < q.length; qi++) {
    const s = q[qi];
    const f = s & 3, k = s >> 2, x = k % W, y = (k - x) / W;
    const preds = [(k << 2) | ((f + 1) & 3), (k << 2) | ((f + 3) & 3)];
    {
      for (const sgn of [1, -1]) {
        if (!passable(k, sgn === 1, f)) continue;
        const px = x - sgn * DV[f][0], py = y - sgn * DV[f][1];
        if (px < 0 || py < 0 || px >= W || py >= H) continue;
        preds.push(((px + py * W) << 2) | f);
      }
    }
    for (const p of preds) {
      if (R[p] >= 0 || !stateOk(p)) continue;
      R[p] = R[s] + 1;
      q.push(p);
    }
  }
  return R;
}

const inb = (w, x, y) => x >= 0 && y >= 0 && x < w.W && y < w.H;

// ------------------------------------------------------------------------------------------------ torchlight
const TL_WALK = new Set([...GROUND, "crystal", "bat", "avatar"]);
let tlRefuel = false;
const SPR = Number(process.env.SPR ?? 2);
function torchlight(w) {
  const { W, av, by, glob, cells } = w;
  const ax = av.location[0], ay = av.location[1], af = fIdx(av.orientation);
  const fuel = glob.fuel;
  // spiders standing in a lit brazier's light never move: ignore them (but don't walk into them)
  const lights = [...(by.brazier_lit ?? []).map((o) => [o.location, 3]), ...(by.sconce_lit ?? []).map((o) => [o.location, 2])];
  const spiders = (by.spider ?? []).filter((sp) => !(sp.variables.stun > 0) && !lights.some(([[bx, by_], r]) => Math.abs(bx - sp.location[0]) <= r && Math.abs(by_ - sp.location[1]) <= r));
  const spCells = new Set((by.spider ?? []).map((o) => o.location[0] + o.location[1] * W));
  // a state is safe when every live spider within 2 (manhattan) is inside the 90-degree view cone
  const stateOk = (s) => {
    const f = s & 3, k = s >> 2, x = k % W, y = (k - x) / W;
    for (const sp of spiders) {
      const dx = sp.location[0] - x, dy = sp.location[1] - y;
      if (Math.abs(dx) + Math.abs(dy) > SPR || (dx === 0 && dy === 0)) continue;
      const fw = dx * DV[f][0] + dy * DV[f][1];
      const side = Math.abs(dx * DV[f][1] - dy * DV[f][0]);
      if (!(fw >= 1 && side <= fw)) return false;
    }
    return true;
  };
  // spider cells can be "entered" going forward when the spider can be driven back (re-planned every tick)
  const open = (k) => (cells.get(k) ?? []).every((o) => TL_WALK.has(o) && o !== "bat");
  const free = (k, fwd, f) => {
    const objs = cells.get(k) ?? [];
    if (objs.every((o) => TL_WALK.has(o))) return true;
    if (!fwd || f === undefined || !objs.includes("spider")) return false;
    const x = k % W, y = (k - x) / W, bx = x + DV[f][0], by_ = y + DV[f][1];
    return inb(w, bx, by_) && open(bx + by_ * W);
  };
  const P = plan(w, free, { x: ax, y: ay, f: af }, stateOk);
  const refuelGoals = new Map();
  for (const name of ["sconce", "sconce_lit", "brazier", "brazier_lit"]) {
    for (const o of by[name] ?? []) {
      const [tx, ty] = o.location;
      for (let f = 0; f < 4; f++) {
        const sx = tx - DV[f][0], sy = ty - DV[f][1];
        const s = ((sx + sy * W) << 2) | f;
        if (inb(w, sx, sy) && free(sx + sy * W) && stateOk(s)) refuelGoals.set(s, 1);
      }
    }
  }
  const R = reverse(w, free, refuelGoals, stateOk);
  const s0 = ((ax + ay * W) << 2) | af;
  const margin = 4;
  if (fuel >= 55) tlRefuel = false;
  if (R[s0] === 1 && fuel < 50) return [MOVE, 2]; // cheap top-up
  let best = null;
  if (!tlRefuel) {
    for (const c of by.crystal ?? []) {
      const k = c.location[0] + c.location[1] * W;
      for (let f = 0; f < 4; f++) {
        const s = (k << 2) | f;
        if (P.dist[s] <= 0) continue;
        const back = by.crystal.length === 1 ? 0 : R[s];
        if (back < 0) continue;
        if (P.dist[s] + back + margin <= fuel && (!best || P.dist[s] < best.d)) best = { d: P.dist[s], a: P.first[s] };
      }
    }
  }
  if (best) { if (DEBUG) console.log('        goal crystal d', best.d); return [MOVE, best.a]; }
  tlRefuel = true;
  if (DEBUG) console.log('        refuel R', R[s0], succ(w, ax, ay, af, free).map(([a, n]) => `${a}:${(n >> 2) % W},${Math.floor((n >> 2) / W)},${n & 3}=${R[n]}/${stateOk(n)}`).join(' '));
  if (R[s0] === 1) return [MOVE, 2];
  if (R[s0] > 1) {
    let bestA = null, bestR = 1e9;
    for (const [a, n] of succ(w, ax, ay, af, free)) if (stateOk(n) && R[n] >= 0 && R[n] < bestR) { bestR = R[n]; bestA = a; }
    if (bestA) return [MOVE, bestA];
  }
  // no safe plan: face the closest live spider, or back away from it
  let near = null;
  for (const sp of spiders) {
    const d = Math.abs(sp.location[0] - ax) + Math.abs(sp.location[1] - ay);
    if (!near || d < near.d) near = { d, sp };
  }
  if (near && near.d <= 3) {
    const dx = near.sp.location[0] - ax, dy = near.sp.location[1] - ay;
    const fw = dx * DV[af][0] + dy * DV[af][1], side = Math.abs(dx * DV[af][1] - dy * DV[af][0]);
    if (!(fw >= 1 && side <= fw)) {
      const want = Math.abs(dx) >= Math.abs(dy) ? (dx > 0 ? 1 : 3) : (dy > 0 ? 2 : 0);
      return [MOVE, ((want - af + 4) % 4) === 1 ? 3 : 1];
    }
    if (fw === 1 && side === 0) return [MOVE, 2]; // drive it back with the torch
    const bx = ax - DV[af][0], by_ = ay - DV[af][1];
    if (inb(w, bx, by_) && free(bx + by_ * W)) return [MOVE, 4];
  }
  return [WAIT, 1];
}

function succ(w, x, y, f, passable) {
  const { W } = w;
  const k = x + y * W;
  const out = [[1, (k << 2) | ((f + 3) & 3)], [3, (k << 2) | ((f + 1) & 3)]];
  for (const [a, sgn] of [[2, 1], [4, -1]]) {
    const nx = x + sgn * DV[f][0], ny = y + sgn * DV[f][1];
    if (inb(w, nx, ny) && passable(nx + ny * W, a === 2, f)) out.push([a, ((nx + ny * W) << 2) | f]);
  }
  return out;
}

// ------------------------------------------------------------------------------------------------ nightgarden
// Guards are deterministic, so the bot simulates them forward (same rules as nightgarden.py / the GDY:
// step every 2nd tick, corner stones turn them, they look 3 tiles ahead and sight stops at blockers) and
// plans in space-time over (x, y, facing, t): never stand in a watched cell outside tall grass, never
// share a cell with a guard. The plan to the nearest lantern (then the gate) is followed until done.
const NG_GROUND = new Set([...GROUND, "turn_right", "turn_left", "turn_back", "tall_grass"]);
const NG_SEE = new Set([...NG_GROUND, "lantern", "avatar"]);
const TURN = { turn_right: 1, turn_left: 3, turn_back: 2 };
const NG_H = 300;
let ngPlan = [];
function simulateGuards(w, ticks) {
  const { W, cells, by } = w;
  const gs = (by.guard ?? []).map((o) => ({ x: o.location[0], y: o.location[1], f: fIdx(o.orientation), mv: o.variables.mv }));
  const at = (k) => (cells.get(k) ?? []).filter((o) => o !== "guard" && o !== "avatar");
  const frames = [];
  for (let t = 0; t < ticks; t++) {
    for (const gd of gs) {
      if (gd.mv === 1) {
        gd.mv = 0;
        const nx = gd.x + DV[gd.f][0], ny = gd.y + DV[gd.f][1];
        const objs = inb(w, nx, ny) ? at(nx + ny * W) : ["wall"];
        const occupied = gs.some((o) => o !== gd && o.x === nx && o.y === ny);
        if (!occupied && objs.every((o) => NG_GROUND.has(o))) {
          gd.x = nx; gd.y = ny;
          const m = objs.find((o) => TURN[o]);
          if (m) gd.f = (gd.f + TURN[m]) & 3;
        } else gd.f = (gd.f + 2) & 3;
      } else gd.mv = 1;
    }
    const pos = new Set(gs.map((o) => o.x + o.y * W));
    const watched = new Set();
    for (const gd of gs) {
      for (let r = 1; r <= 3; r++) {
        const x = gd.x + DV[gd.f][0] * r, y = gd.y + DV[gd.f][1] * r;
        if (!inb(w, x, y)) break;
        const k = x + y * W;
        if (pos.has(k)) break;
        watched.add(k);
        if (!at(k).every((o) => NG_SEE.has(o))) break;
      }
    }
    frames.push({ pos, watched });
  }
  return frames;
}

function nightgarden(w) {
  if (ngPlan.length) return ngPlan.shift();
  const { W, H, av, by, cells, glob } = w;
  const frames = simulateGuards(w, NG_H);
  const now = new Set((by.guard ?? []).map((o) => o.location[0] + o.location[1] * W));
  const grass = (k) => (cells.get(k) ?? []).includes("tall_grass");
  const walkable = (k) => (cells.get(k) ?? []).every((o) => NG_GROUND.has(o) || o === "lantern" || o === "avatar");
  const lanterns = new Set((by.lantern ?? []).map((o) => o.location[0] + o.location[1] * W));
  const gate = (by.gate ?? [])[0];
  const gateK = gate ? gate.location[0] + gate.location[1] * W : -1;
  const wantGate = glob.need === 0;
  // BFS over (k, f, t)
  const S = W * H * 4;
  const seen = new Uint8Array(S * (NG_H + 1));
  const prev = new Int32Array(S * (NG_H + 1)).fill(-1);
  const act = new Int8Array(S * (NG_H + 1));
  const start = ((av.location[0] + av.location[1] * W) << 2) | fIdx(av.orientation);
  let layer = [start];
  seen[start] = 1;
  for (let t = 0; t < NG_H && layer.length; t++) {
    const fr = frames[t];
    const prevPos = t === 0 ? now : frames[t - 1].pos;
    const next = [];
    for (const s of layer) {
      const f = s & 3, k = s >> 2, x = k % W, y = (k - x) / W;
      const cand = [[0, s], [1, (k << 2) | ((f + 3) & 3)], [3, (k << 2) | ((f + 1) & 3)]];
      for (const [a, sgn] of [[2, 1], [4, -1]]) {
        const nx = x + sgn * DV[f][0], ny = y + sgn * DV[f][1];
        if (!inb(w, nx, ny)) continue;
        const nk = nx + ny * W;
        if (a === 2 && wantGate && nk === gateK) {
          // escape!
          const out = [[MOVE, 2]];
          let cs = s, ct = t;
          while (ct > 0) { const id = cs + S * ct; out.unshift(act[id] === 0 ? [WAIT, 1] : [MOVE, act[id]]); cs = prev[id]; ct--; }
          ngPlan = out;
          return ngPlan.shift();
        }
        if (walkable(nk) && !prevPos.has(nk)) cand.push([a, (nk << 2) | f]);
      }
      for (const [a, n] of cand) {
        const nk = n >> 2;
        if (fr.pos.has(nk) || (fr.watched.has(nk) && !grass(nk))) continue;
        const id = n + S * (t + 1);
        if (seen[id]) continue;
        seen[id] = 1; prev[id] = s; act[id] = a;
        if (!wantGate && lanterns.has(nk)) {
          const out = [];
          let cs = n, ct = t + 1;
          while (ct > 0) { const i2 = cs + S * ct; out.unshift(act[i2] === 0 ? [WAIT, 1] : [MOVE, act[i2]]); cs = prev[i2]; ct--; }
          ngPlan = out;
          return ngPlan.shift();
        }
        next.push(n);
      }
    }
    layer = next;
  }
  return [WAIT, 1];
}

// ------------------------------------------------------------------------------------------------ main
const POLICIES = { torchlight, nightgarden };
const policy = POLICIES[id];
if (!policy) throw new Error(`no policy for ${id}`);

const recorded = [];
const n = g.getLevelCount();
let allWon = true;
for (let l = 0; l < n; l++) {
  if (onlyLevel !== null && l !== onlyLevel) continue;
  g.loadLevel(l);
  let wins = 0, steps = [];
  const fails = {};
  for (let ep = 0; ep < episodes; ep++) {
    tlRefuel = false;
    ngPlan = [];
    g.seed(ep + 1);
    g.reset();
    let res = "timeout";
    const taken = [];
    rngState = 7919 * (ep + 1);
    const seen = new Map();
    for (let t = 0; t < maxSteps; t++) {
      const w = world();
      if (!w.av) { res = "dead"; break; }
      let a = policy(w);
      // loop breaking: a pose visited many times (or epsilon) -> a random move
      const key = `${w.av.location}:${w.av.orientation}`;
      seen.set(key, (seen.get(key) ?? 0) + 1);
      if (id === "torchlight" && (rand() < EPS || seen.get(key) > 6)) { a = [MOVE, 1 + Math.floor(rand() * 4)]; seen.set(key, 0); }
      taken.push(a);
      if (process.argv.includes("-vv")) console.log(`      t=${t} av=${w.av.location} ${w.av.orientation} fuel=${w.glob.fuel} a=${a} sp=${(w.by.spider ?? []).map((o) => o.location + ":" + o.renderTileId).join(" ")} bats=${(w.by.bat ?? []).map((o) => o.location).join(" ")}`);
      const r = g.step(a);
      if (r && r.terminated) {
        res = r.info?.playerResults?.["1"] === "Win" ? "win" : "lose";
        if (res === "lose" && verbose) {
          const w2 = world();
          console.log(`    L${l} ep${ep} lost at t=${t}: fuel=${w2.glob.fuel} avatar=${w.av.location} ${w.av.orientation} act=${a}`);
        }
        steps.push(t + 1);
        break;
      }
    }
    if (res === "win") {
      wins++;
      if (record && !recorded[l]) recorded[l] = { seed: ep + 1, actions: taken };
    } else fails[res] = (fails[res] ?? 0) + 1;
  }
  if (wins < episodes) allWon = false;
  console.log(`  L${l}: ${g.width}x${g.height} bot win ${wins}/${episodes} ${JSON.stringify(fails)} steps ${steps.join(",")}`);
}
if (record) {
  const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions", `${id}.json`);
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
process.exit(allWon ? 0 : 1);
