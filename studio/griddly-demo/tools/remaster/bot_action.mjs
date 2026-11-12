// Cautious BFS bot for the action remasters (zelda, butterflies, spider_nest):
//   node studio/griddly-demo/tools/remaster/bot_action.mjs <game_id> [episodes] [maxSteps] [level]
//
// Every step it reads getState() and:
//   * (zelda) attacks an enemy standing next to the avatar;
//   * otherwise BFS-es from the avatar to the nearest goal cell, avoiding enemy cells and the cells
//     next to them (danger); if no safe route exists it waits, or steps to the neighbouring cell
//     furthest from the enemies when one is adjacent;
// and reports per-level win / lose / timeout using the engine's own termination.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const GROUND = ["grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge"];
const GAMES = {
  zelda: {
    walk: [...GROUND, "attack_fire"],
    enemies: ["spider", "bat"],
    goals: (a) => ["key", ...(a.variables.keys > 0 ? ["door", "exit"] : [])],
    attack: true,
  },
  butterflies: { walk: GROUND, enemies: ["spider"], goals: () => ["butterfly"] },
  spider_nest: { walk: GROUND, enemies: ["spider"], goals: () => ["gem"] },
  spiders: { walk: GROUND, enemies: ["spider"], goals: (a) => ["gem", "key", ...(a.variables.keys > 0 ? ["door"] : [])] },
};

const id = process.argv[2];
const episodes = Number(process.argv[3] ?? 10);
const maxSteps = Number(process.argv[4] ?? 1500);
const posArgs = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const onlyLevel = posArgs[3] !== undefined ? Number(posArgs[3]) : null;
const cfg = GAMES[id];
// `spiders` (Spiders Garden) predates the remasters and lives in Mini-Grid/
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
let waited = 0; // steps spent waiting for a safe route; after a while the bot takes riskier routes
const enemySet = new Set(cfg.enemies);

function decide() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const cells = new Map();
  let av = null;
  const foes = [];
  for (const o of s.objects) {
    if (o.name === avatarName) { av = o; continue; }
    const k = o.location[0] + o.location[1] * W;
    if (!cells.has(k)) cells.set(k, []);
    cells.get(k).push(o.name);
    if (enemySet.has(o.name)) foes.push(o.location);
  }
  if (!av) return null;
  const [ax, ay] = av.location;
  const inb = (x, y) => x >= 0 && y >= 0 && x < W && y < H;
  const danger = new Set();
  for (const [ex, ey] of foes) {
    danger.add(ex + ey * W);
    for (const [, dx, dy] of DIRS) if (inb(ex + dx, ey + dy)) danger.add(ex + dx + (ey + dy) * W);
  }
  const free = (k) => (cells.get(k) ?? []).every((o) => walk.has(o));
  // zelda: strike an adjacent enemy
  if (cfg.attack) {
    for (const [a, dx, dy] of DIRS) {
      const objs = cells.get(ax + dx + (ay + dy) * W) ?? [];
      if (objs.some((o) => enemySet.has(o))) return ["attack", a];
    }
    // an enemy two cells away in line: put a flame between us (enemies walking into it die)
    for (const [a, dx, dy] of DIRS) {
      const mid = ax + dx + (ay + dy) * W, far = ax + 2 * dx + (ay + 2 * dy) * W;
      if (!inb(ax + 2 * dx, ay + 2 * dy) || !free(mid)) continue;
      if ((cells.get(mid) ?? []).includes("attack_fire")) continue;
      if ((cells.get(far) ?? []).some((o) => enemySet.has(o))) return ["attack", a];
    }
  }
  const goals = new Set(cfg.goals(av));
  const bfs = (avoid) => {
    const start = ax + ay * W;
    const prev = new Map([[start, -1]]);
    const q = [start];
    for (let i = 0; i < q.length; i++) {
      const c = q[i];
      const x = c % W, y = (c - x) / W;
      for (const [a, dx, dy] of DIRS) {
        const nx = x + dx, ny = y + dy;
        if (!inb(nx, ny)) continue;
        const n = nx + ny * W;
        if (prev.has(n)) continue;
        const objs = cells.get(n) ?? [];
        if (avoid.has(n) && !(c === start && false)) continue;
        if (objs.some((o) => goals.has(o))) {
          let cur = c, step = a;
          while (prev.get(cur) !== -1) {
            const p = prev.get(cur);
            const px = p % W;
            step = DIRS.find(([, ddx, ddy]) => px + ddx === cur % W && (p - px) / W + ddy === (cur - (cur % W)) / W)[0];
            cur = p;
          }
          return step;
        }
        if (free(n)) { prev.set(n, c); q.push(n); }
      }
    }
    return 0;
  };
  const adjacentFoe = foes.some(([ex, ey]) => Math.abs(ex - ax) + Math.abs(ey - ay) <= 1);
  const step = bfs(danger);
  if (step) { waited = 0; return ["move", step]; }
  // armed (zelda): push through, only avoiding the enemies' own cells; it strikes when adjacent
  if (cfg.attack || waited > 12) {
    const occupied = new Set(foes.map(([ex, ey]) => ex + ey * W));
    const bold = bfs(occupied);
    if (bold) return ["move", bold];
  }
  if (adjacentFoe || danger.has(ax + ay * W)) {
    // retreat: neighbouring free cell maximising distance to the nearest enemy
    let best = 0, bestD = -1;
    for (const [a, dx, dy] of DIRS) {
      const nx = ax + dx, ny = ay + dy;
      if (!inb(nx, ny) || !free(nx + ny * W) || foes.some(([ex, ey]) => ex === nx && ey === ny)) continue;
      const d = Math.min(...foes.map(([ex, ey]) => Math.abs(ex - nx) + Math.abs(ey - ny)));
      if (d > bestD) { bestD = d; best = a; }
    }
    if (best) return ["move", best];
  }
  waited++;
  return ["move", 0]; // wait for the enemies to move away
}

// --record: save the first winning episode per level (seed + actions) to apps/web/src/games/solutions/<id>.json, in the
// replay format the demo's "Watch solution" uses: {format: 2, levels: [{seed, actions: [[a, id], ...]}]}.
const record = process.argv.includes("--record");
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
      taken.push(act(d[0], d[1]));
      const r = g.step(act(d[0], d[1]));
      if (r && r.terminated) {
        res = r.info?.playerResults?.["1"] === "Win" ? "win" : "lose";
        steps.push(t + 1);
        break;
      }
    }
    if (res === "timeout" && process.env.DEBUG) {
      const st = g.getState();
      const a = st.objects.find((o) => o.name === avatarName);
      const cnt = {};
      for (const o of st.objects) cnt[o.name] = (cnt[o.name] ?? 0) + 1;
      console.log(`    ep${ep} timeout at ${a?.location} vars=${JSON.stringify(a?.variables)} last=${JSON.stringify(decide())}`,
        JSON.stringify(Object.fromEntries(cfg.enemies.concat(["key", "door", "exit", "gem", "butterfly", "cocoon", "nest"]).map((k) => [k, cnt[k] ?? 0]))));
    }
    if (res === "timeout" && process.env.DEBUG > 1) {
      const st = g.getState();
      const ch = { avatar: "A", spider: "S", bat: "b", key: "k", door: "D", exit: "E", wall: "W", nest: "N", gem: "g", butterfly: "f", cocoon: "c", gnome: "A", catcher: "A", lava: "L" };
      const grid = Array.from({ length: g.height }, () => Array(g.width).fill("."));
      for (const o of st.objects.sort((a, b) => a.zidx - b.zidx)) if (o.location[0] < 0) console.log("OFFGRID", JSON.stringify(o)); else if (!GROUND.includes(o.name)) grid[o.location[1]][o.location[0]] = ch[o.name] ?? "#";
      console.log(grid.map((r) => r.join("")).join("\n"));
    }
    if (res === "win" && record && !recorded[l]) recorded[l] = { seed: ep + 1, actions: taken.map((x) => (x.length === 1 ? [0, x[0]] : x)) };
    if (res === "win") wins++;
    else if (res === "lose") losses++;
    else timeouts++;
  }
  total += episodes;
  totalWins += wins;
  console.log(`  L${l}: ${g.width}x${g.height} bot win ${wins} lose ${losses} timeout ${timeouts} / ${episodes}  steps ${steps.join(",")}`);
}
console.log(`${id}: bot won ${totalWins}/${total}`);

if (record) {
  const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions", `${id}.json`);
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
