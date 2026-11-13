// Planning bot for Deep Delve (minecrawler):
//   node studio/griddly-demo/tools/remaster/bot_minecrawler.mjs [episodes] [maxSteps] [level] [--record]
//
// Every step it reads getState() and runs Dijkstra from the miner over the grid, where walking costs
// 1, dirt / ore veins / cracked rock 2 (dig + step) and rock 3. It plays it safe:
//   * a "hazard" cell (below a boulder or next to lava) is only opened when there is no other way and
//     it carries timber: dig it, then at once `build` a support beam into it, then walk through;
//   * never steps on an unbraced cell next to lava, nor below a wobbling or rolling boulder.
// --naive ignores all of that (shows the hazards matter).
// Goals, in order: eat a ration when stamina will not last; fill the nearest empty cart when carrying
// enough ore; otherwise collect ore (loose ore or veins); when every cart is full, take the lift.
// --record writes the first winning run of every level to apps/web/src/games/solutions/minecrawler.json.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const pos = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const episodes = Number(pos[0] ?? 5);
const maxSteps = Number(pos[1] ?? 2000);
const onlyLevel = pos[2] !== undefined ? Number(pos[2]) : null;
const record = process.argv.includes("--record");
const NAIVE = process.argv.includes("--naive"); // ignore boulders and lava (shows the hazards matter)
const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered/minecrawler.yaml"), "utf8");
const CART_ORE = Number(gdy.match(/gte: \[src\.ore, (\d+)\]/)[1]);
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const MOVE = actions.indexOf("move"), DIG = actions.indexOf("dig"), BUILD = actions.indexOf("build");
const DIRS = [[1, -1, 0], [2, 0, -1], [3, 1, 0], [4, 0, 1]];
const PASS = new Set(["tunnel", "grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge", "beam"]);
const PICK = new Set(["ore", "timber", "ration"]);

function snapshot() {
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const top = new Map();
  const cellObjs = new Map();
  let av = null;
  for (const o of s.objects) {
    if (o.location[0] < 0) continue;
    if (o.name === "miner") { av = o; continue; }
    const k = o.location[0] + o.location[1] * W;
    if (!cellObjs.has(k)) cellObjs.set(k, new Set());
    cellObjs.get(k).add(o.name);
    const cur = top.get(k);
    if (!cur || o.zidx > cur.zidx) top.set(k, o);
  }
  const gv = g.getGlobalVariables();
  const v = gv.carts_needed;
  return { W, H, top, cellObjs, av, carts: typeof v === "object" ? Number(Object.values(v)[0]) : Number(v) };
}

function decide() {
  const st = snapshot();
  const { W, H, top, cellObjs, av } = st;
  if (!av) return null;
  const name = (k) => top.get(k)?.name ?? "_empty";
  const at = (x, y) => (x < 0 || y < 0 || x >= W || y >= H ? "bedrock" : name(x + y * W));
  const timber = av.variables.timber;
  const beamAt = (x, y) => [...(cellObjs.get(x + y * W) ?? [])].includes("beam");
  const lavaNext = (x, y) => DIRS.some(([, dx, dy]) => at(x + dx, y + dy) === "lava");
  // a hazard cell: once open it lets a boulder fall into it or lava ooze in, unless braced by a beam
  const hazard = (x, y) => !beamAt(x, y) && (at(x, y - 1) === "boulder" || lavaNext(x, y));
  // danger map: open cells next to lava (unbraced) and the column below a wobbling / rolling boulder
  const danger = new Set();
  for (const [k, o] of top) {
    const x = k % W, y = (k - x) / W;
    if (o.name === "lava") for (const [, dx, dy] of DIRS) if (!beamAt(x + dx, y + dy)) danger.add(x + dx + (y + dy) * W);
    if (o.name === "boulder" && o.variables.falling > 0) {
      for (let yy = y + 1; yy < H; yy++) {
        const n = at(x, yy);
        if (!(n === "_empty" || PASS.has(n) || n === "miner") || beamAt(x, yy)) break;
        danger.add(x + yy * W);
      }
    }
  }
  const stepCost = (x, y) => {
    const k = x + y * W;
    const n = at(x, y);
    const open = n === "_empty" || PASS.has(n) || PICK.has(n);
    const diggable = n === "dirt_block" || n === "vein" || n === "rock_block";
    if (!open && !diggable) return Infinity;
    const dig = !diggable ? 0 : n === "rock_block" && top.get(k).variables.hp >= 2 ? 2 : 1;
    if (!NAIVE && hazard(x, y)) {
      // braceable: dig it, build a beam, step in (only worth it when there is no other way)
      const above = top.get(x + (y - 1) * W);
      if (timber < 1 || (above?.name === "boulder" && above.variables.falling === 1)) return Infinity;
      return 1 + dig + 1 + 60;
    }
    if (danger.has(k) && !NAIVE) return Infinity;
    return 1 + dig;
  };
  // Dijkstra
  const start = av.location[0] + av.location[1] * W;
  const dist = new Map([[start, 0]]), prev = new Map([[start, -1]]), digs = new Map([[start, 0]]);
  const pq = [[0, start]];
  while (pq.length) {
    let bi = 0;
    for (let i = 1; i < pq.length; i++) if (pq[i][0] < pq[bi][0]) bi = i;
    const [d, c] = pq.splice(bi, 1)[0];
    if (d > dist.get(c)) continue;
    const x = c % W, y = (c - x) / W;
    for (const [, dx, dy] of DIRS) {
      const nx = x + dx, ny = y + dy;
      const n = nx + ny * W;
      const cost = stepCost(nx, ny);
      if (!isFinite(cost)) continue;
      if (d + cost < (dist.get(n) ?? Infinity)) {
        dist.set(n, d + cost);
        prev.set(n, c);
        digs.set(n, digs.get(c) + (cost > 1 ? cost - 1 : 0));
        pq.push([d + cost, n]);
      }
    }
  }
  // best target: a cell from which a goal object is adjacent (or the goal cell itself for walk-ins)
  const stamina = av.variables.stamina, ore = av.variables.ore;
  const goalsFor = () => {
    const out = [];
    for (const [k, o] of top) {
      const x = k % W, y = (k - x) / W;
      if (o.name === "ration" && stamina < 16) out.push([k, -30]);
      if (o.name === "timber") out.push([k, 6]);
      if (st.carts > 0 && ore >= CART_ORE && o.name === "cart") out.push([k, 0, "adj"]);
      if (st.carts > 0 && ore < CART_ORE && o.name === "ore") out.push([k, 0]);
      if (st.carts > 0 && ore < CART_ORE && o.name === "vein" && at(x, y - 1) !== "boulder" && !danger.has(k)) out.push([k, 1]);
      if (st.carts <= 0 && o.name === "lift") out.push([k, 0, "adj"]);
    }
    return out;
  };
  let best = null;
  for (const [k, bias, mode] of goalsFor()) {
    const x = k % W, y = (k - x) / W;
    if (mode === "adj") {
      for (const [, dx, dy] of DIRS) {
        const n = x + dx + (y + dy) * W;
        if (!dist.has(n)) continue;
        const sc = dist.get(n) + bias;
        if (!best || sc < best.sc) best = { sc, cell: n, bump: k };
      }
    } else if (dist.has(k)) {
      const sc = dist.get(k) + bias;
      if (!best || sc < best.sc) best = { sc, cell: k };
    }
  }
  if (!best) return [MOVE, 0];
  // walk the path back to the first step
  let first = best.cell;
  if (first === start && best.bump !== undefined) first = best.bump;
  else {
    while (prev.get(first) !== start && prev.get(first) !== -1) first = prev.get(first);
  }
  if (first === start) return [MOVE, 0];
  const fx = first % W, fy = (first - fx) / W;
  const dir = DIRS.find(([, dx, dy]) => av.location[0] + dx === fx && av.location[1] + dy === fy)[0];
  const n = at(fx, fy);
  if (n === "dirt_block" || n === "vein" || n === "rock_block") return [DIG, dir];
  if (!NAIVE && hazard(fx, fy) && timber > 0 && (n === "_empty" || PASS.has(n))) return [BUILD, dir];
  return [MOVE, dir];
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
    if (res !== "win" && process.env.DEBUG) {
      const st = snapshot();
      console.log(`    ep${ep} ${res}: miner ${st.av?.location} ${JSON.stringify(st.av?.variables)} carts_needed ${st.carts} last ${JSON.stringify(taken.slice(-6))}`);
    }
    if (process.env.DEBUG) console.log(`    ep${ep} ${res} final ${JSON.stringify(snapshot().av?.variables)}`);
    if (res === "win" && record && !recorded[l]) recorded[l] = { seed: ep + 1, actions: taken };
    if (res === "win") wins++;
    else if (res === "lose") losses++;
    else timeouts++;
  }
  total += episodes;
  totalWins += wins;
  console.log(`  L${l}: ${g.width}x${g.height} bot win ${wins} lose ${losses} timeout ${timeouts} / ${episodes}  steps ${steps.join(",")}`);
}
console.log(`minecrawler: bot won ${totalWins}/${total}`);
if (record) {
  const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
  if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
  const out = path.join(here, "../../apps/web/src/games/solutions/minecrawler.json");
  fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
  console.log(`record: wrote ${out}`);
}
