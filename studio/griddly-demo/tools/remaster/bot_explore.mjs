// Greedy BFS bot for the exploration remasters (doggo, eyeball, drunkdwarf, labyrinth):
//   node studio/griddly-demo/tools/remaster/bot_explore.mjs <game_id> [episodes] [maxSteps]
//
// Each step it reads getState(), BFS-es over 4-connected cells from the avatar and walks towards the
// nearest "goal" cell (bumping/stepping into it). A cell is walkable when every object on it is in
// the game's `walk` set; goals depend on the game (and the avatar's variables, e.g. keys). Reports
// per-level win rate using the engine's own termination.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const GROUND = ["grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge"];
const GAMES = {
  doggo: { walk: GROUND, goals: () => ["stick"] },
  eyeball: { walk: GROUND, goals: () => ["eye_drops"] },
  drunkdwarf: {
    walk: [...GROUND, "open_door"],
    goals: (a) => ["key", "coffin_bed", ...(a.variables.keys > 0 ? ["door"] : [])],
  },
  labyrinth: { walk: GROUND, goals: () => ["exit"] },
};

const id = process.argv[2];
const episodes = Number(process.argv[3] ?? 3);
const maxSteps = Number(process.argv[4] ?? 2000);
const cfg = GAMES[id];
const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered", `${id}.yaml`), "utf8");
const avatarName = gdy.match(/AvatarObject:\s*(\S+)/)[1];
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const act = (name, a) => (actions.length > 1 ? [actions.indexOf(name), a] : [a]);
const DIRS = [[1, -1, 0], [2, 0, -1], [3, 1, 0], [4, 0, 1]];

function plan() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const cells = new Map();
  let av = null;
  for (const o of s.objects) {
    if (o.name === avatarName) { av = o; continue; }
    const k = o.location[0] + o.location[1] * W;
    if (!cells.has(k)) cells.set(k, []);
    cells.get(k).push(o.name);
  }
  if (!av) return null;
  const goals = new Set(cfg.goals(av));
  const walk = new Set(cfg.walk);
  const start = av.location[0] + av.location[1] * W;
  const prev = new Map([[start, -1]]);
  const q = [start];
  while (q.length) {
    const c = q.shift();
    const x = c % W, y = (c - x) / W;
    for (const [a, dx, dy] of DIRS) {
      const nx = x + dx, ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
      const n = nx + ny * W;
      if (prev.has(n)) continue;
      const objs = cells.get(n) ?? [];
      if (objs.some((o) => goals.has(o))) {
        // walk back to the first step
        let cur = c, step = a;
        while (prev.get(cur) !== -1) {
          const p = prev.get(cur);
          const px = p % W, py = (p - px) / W;
          step = DIRS.find(([, ddx, ddy]) => px + ddx === cur % W && py + ddy === (cur - (cur % W)) / W)[0];
          cur = p;
        }
        return step;
      }
      if (objs.every((o) => walk.has(o))) {
        prev.set(n, c);
        q.push(n);
      }
    }
  }
  return 0; // no goal reachable
}

// --record: save the first winning episode per level (seed + actions) to apps/web/src/games/solutions/<id>.json, in the
// replay format the demo's "Watch solution" uses: {format: 2, levels: [{seed, actions: [[a, id], ...]}]}.
const record = process.argv.includes("--record");
const recorded = [];
const n = g.getLevelCount();
let allWon = true;
for (let l = 0; l < n; l++) {
  g.loadLevel(l);
  let wins = 0, stuck = 0, steps = [];
  for (let ep = 0; ep < episodes; ep++) {
    g.seed(ep + 1);
    g.reset();
    let res = "timeout";
    const taken = [];
    for (let t = 0; t < maxSteps; t++) {
      const a = plan();
      if (!a) { res = a === 0 ? "stuck" : "dead"; break; }
      taken.push(act("move", a));
      const r = g.step(act("move", a));
      if (r && r.terminated) {
        res = r.info?.playerResults?.["1"] === "Win" ? "win" : "lose";
        steps.push(t + 1);
        break;
      }
    }
    if (res === "win" && record && !recorded[l]) recorded[l] = { seed: ep + 1, actions: taken.map((x) => (x.length === 1 ? [0, x[0]] : x)) };
    if (res === "win") wins++;
    else { stuck++; console.log(`    L${l} ep${ep}: ${res}`); }
  }
  if (wins < episodes) allWon = false;
  console.log(`  L${l}: ${g.width}x${g.height} bot win ${wins}/${episodes} steps ${steps.join(",")}`);
}
if (record) {
  const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions", `${id}.json`);
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
process.exit(allWon ? 0 : 1);
