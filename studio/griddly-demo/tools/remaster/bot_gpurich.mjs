// Planning bot for GPU Rich (gpurich):
//   node studio/griddly-demo/tools/remaster/bot_gpurich.mjs [episodes] [maxSteps] [level] [--record]
//
// Each step it reads getState() and follows a fixed economy plan:
//   1. carrying enough tokens to hit the target, or a full pocket (>= 8), or carrying some and no token
//      is lying around -> walk into the Mistral HQ;
//   2. holding >= 2 mayo and a "cool" pad exists (reachable side, at most 1 live GPU neighbour, so
//      nothing ever overheats) and fewer than `maxGpus` GPUs -> walk next to it and `build` facing it;
//   3. a token lies within reach -> pick up the nearest (racing le chaton fat);
//   4. fewer than 2 mayo and still building -> pick up the nearest mayo jar;
//   5. otherwise wait next to the GPUs.
// --record writes the first winning run per level to apps/web/src/games/solutions/gpurich.json
// (format 2, replayed by replay_solutions.mjs).
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const pos = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const episodes = Number(pos[0] ?? 10);
const maxSteps = Number(pos[1] ?? 1500);
const onlyLevel = pos[2] !== undefined ? Number(pos[2]) : null;
const record = process.argv.includes("--record");
const maxGpus = Number((process.argv.find((a) => a.startsWith("--gpus=")) ?? "=10").split("=")[1]);
const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered/gpurich.yaml"), "utf8");
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const MOVE = actions.indexOf("move"), BUILD = actions.indexOf("build");
const DIRS = [[1, -1, 0], [2, 0, -1], [3, 1, 0], [4, 0, 1]];
const GROUND = new Set(["grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge"]);
const gval = (gv, k) => { const v = gv[k]; return typeof v === "object" ? Number(Object.values(v)[0]) : Number(v); };

function decide() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const top = new Map();
  let av = null;
  for (const o of s.objects) {
    if (o.location[0] < 0 || GROUND.has(o.name)) continue;
    if (o.name === "bufo") { av = o; continue; }
    top.set(o.location[0] + o.location[1] * W, o.name);
  }
  if (!av) return null;
  const gv = g.getGlobalVariables();
  const tokens = gval(gv, "tokens"), target = gval(gv, "target"), gpus = gval(gv, "gpus");
  const { mayo, carry } = av.variables;
  const [ax, ay] = av.location;
  const at = (x, y) => (x < 0 || y < 0 || x >= W || y >= H ? "wall" : top.get(x + y * W));
  const walk = (x, y) => {
    const o = at(x, y);
    return o === undefined || (o === "token" && carry < 12) || (o === "jar" && mayo < 6);
  };
  // BFS to a cell from which `want(x, y)` returns a direction (bump / build into a neighbour), or a cell
  // where `here(x, y)` holds. Returns [actionType, dir].
  const go = (want, kind = MOVE, maxDepth = 9999) => {
    const start = ax + ay * W;
    const prev = new Map([[start, [-1, 0]]]);
    const depth = new Map([[start, 0]]);
    const q = [start];
    for (let i = 0; i < q.length; i++) {
      const c = q[i];
      const x = c % W, y = (c - x) / W;
      if (depth.get(c) > maxDepth) break;
      const d = want(x, y);
      if (d) {
        if (c === start) return [kind, d];
        let cur = c;
        while (prev.get(cur)[0] !== start) cur = prev.get(cur)[0];
        return [MOVE, prev.get(cur)[1]];
      }
      for (const [a, dx, dy] of DIRS) {
        const nx = x + dx, ny = y + dy, n = nx + ny * W;
        if (prev.has(n) || !walk(nx, ny)) continue;
        prev.set(n, [c, a]);
        depth.set(n, depth.get(c) + 1);
        q.push(n);
      }
    }
    return null;
  };
  const adjTo = (name, cond = () => true) => (x, y) => {
    for (const [a, dx, dy] of DIRS) if (at(x + dx, y + dy) === name && cond(x + dx, y + dy)) return a;
    return 0;
  };
  const anyTokens = [...top.values()].includes("token");
  const need = target - tokens;
  if (carry > 0 && (carry >= need || carry >= 8 || !anyTokens)) {
    const r = go(adjTo("hq"));
    if (r) return r;
  }
  const gpuNbrs = (x, y) => DIRS.filter(([, dx, dy]) => at(x + dx, y + dy) === "gpu").length;
  const coolPad = (x, y) => gpuNbrs(x, y) <= 1;
  // build phase: get to `want` GPUs first (tokens only when they lie right next to the route)
  const want = Math.min(maxGpus, Math.ceil(target / 10) + 2);
  if (gpus < want) {
    if (anyTokens && carry < 12) {
      const r = go(adjTo("token"), MOVE, 3);
      if (r) return r;
    }
    if (mayo >= 2) {
      const r = go(adjTo("pad", coolPad), BUILD);
      if (r) return r;
    }
    if (mayo < 6) {
      const r = go(adjTo("jar"));
      if (r) return r;
    }
  }
  if (anyTokens && carry < 12) {
    const r = go(adjTo("token"));
    if (r) return r;
  }
  if (mayo >= 2 && gpus < maxGpus) {
    const r = go(adjTo("pad", coolPad), BUILD);
    if (r) return r;
  }
  if (carry > 0) {
    const r = go(adjTo("hq"));
    if (r) return r;
  }
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
      const cnt = {};
      for (const o of g.getState().objects) cnt[o.name] = (cnt[o.name] ?? 0) + 1;
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
console.log(`gpurich: bot won ${totalWins}/${total}`);

if (record) {
  const missing = [...Array(n).keys()].filter((l) => onlyLevel === null && !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions/gpurich.json");
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
