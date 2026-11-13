// Cautious BFS bot for the action remasters (zelda, butterflies, spider_nest) and Spiders Garden:
//   node studio/griddly-demo/tools/remaster/bot_action.mjs <game_id> [episodes] [maxSteps] [level]
//        [--naive] [--search] [--record] [--seed0=N]
//
// Every step it reads getState() and:
//   * (zelda) attacks an enemy standing next to the avatar, or puts a sword flash between itself and an
//     enemy two cells away in line;
//   * wandering enemies (spider, bat): avoids their cells and the cells next to them;
//   * hunters (A* chasers, see action_kit.hunter): computes each hunter's BFS distance map over the cells
//     it can walk, and only plans routes where the avatar reaches every cell before any hunter could
//     (hunter steps by tick k = ceil(k / pace) + margin). If no goal is safely reachable it kites: it
//     heads for the safely reachable cell furthest from the hunters (loops make that work), preferring
//     cells that are not dead ends;
//   * otherwise BFS-es to the nearest goal; waits / retreats when nothing is safe.
// --naive   the old cautious bot: hunters are treated like wandering spiders (no time model) - a
//           difficulty baseline.
// --search  recorder mode: when the avatar dies, rewind (reset + replay the action prefix - the engine is
//           deterministic for a seed) a few steps back and continue with a random perturbation; the
//           rewind depth grows with repeated deaths. This is a search over the exact engine state.
// --record  save the first winning episode per level (seed + actions) to apps/web/src/games/solutions/<id>.json
//           in the replay format the demo's "Watch solution" uses: {format: 2, levels: [{seed, actions}]}.
//           With [level], only that level's entry is replaced in an existing file.
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
    walkOn: ["key"],
    enemies: ["spider", "bat"],
    hunters: ["hunter"],
    goals: (a) => ["key", ...(a.variables.keys > 0 ? ["door", "exit"] : [])],
    attack: true,
  },
  butterflies: { walk: GROUND, walkOn: ["butterfly"], enemies: ["spider"], hunters: ["hunter"], goals: () => ["butterfly"] },
  spider_nest: { walk: GROUND, enemies: ["spider"], hunters: ["hunter"], goals: () => ["gem"] },
  spiders: { walk: GROUND, enemies: ["spider"], hunters: ["hunter"],
    goals: (a) => ["gem", "key", ...(a.variables.keys > 0 ? ["door"] : [])] },
};

const id = process.argv[2];
const posArgs = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const episodes = Number(posArgs[1] ?? 10);
const maxSteps = Number(posArgs[2] ?? 1500);
const onlyLevel = posArgs[3] !== undefined ? Number(posArgs[3]) : null;
const naive = process.argv.includes("--naive");
const search = process.argv.includes("--search");
const record = process.argv.includes("--record");
const seed0 = Number((process.argv.find((a) => a.startsWith("--seed0=")) ?? "--seed0=1").slice(8));
const MARGIN = Number(process.env.MARGIN ?? 0);
const STRICT = process.env.STRICT === "1";
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
const enemySet = new Set(naive ? [...cfg.enemies, ...cfg.hunters] : cfg.enemies);
const hunterSet = new Set(naive ? [] : cfg.hunters);
const killers = new Set([...cfg.enemies, ...cfg.hunters]);
let waited = 0; // steps spent waiting for a safe route; after a while the bot takes riskier routes
// tiny seeded PRNG (perturbations in --search)
let rngState = 1;
const rand = () => ((rngState = (rngState * 1103515245 + 12345) >>> 0) / 4294967296);
let perturb = 0; // > 0: take random safe-ish actions for this many decisions

// Hunter-aware veto. Hunters are modelled exactly enough: each steps 1 cell along a shortest path to the
// avatar at ticks due, due + pace, ... (stalk publishes `due` = the tick of its next step, `pace`), never
// into other hunters. survival(first) runs a space-time search over avatar moves (wait / 4 dirs) for
// ROLL ticks after the candidate first move, with the hunters reacting deterministically, and returns
// the longest survival found (ROLL = the avatar can provably stay alive that long under the model).
const ROLL = Number(process.env.ROLL ?? 12);
// hunter radii, read from the GDY's stalk action (lte: [d, SENSE] / gt: [d, LOSE])
const SENSE = Number(gdy.match(/lte: \[d, (\d+)\]/)?.[1] ?? 9);
const LOSE = Number(gdy.match(/gt: \[d, (\d+)\]/)?.[1] ?? 14);
function survival(ctx, first) {
  const { W, H, start, hunters, free, hfree, danger, now } = ctx;
  const inb = (x, y) => x >= 0 && y >= 0 && x < W && y < H;
  const NB = ctx.NB ??= new Map();
  const nb = (c) => {
    let r = NB.get(c);
    if (r) return r;
    const x = c % W, y = (c - x) / W;
    r = [];
    for (const [, dx, dy] of DIRS) if (inb(x + dx, y + dy)) r.push(x + dx + (y + dy) * W);
    NB.set(c, r);
    return r;
  };
  const DM = ctx.DM ??= new Map(); // distance-to-avatar maps (hunters walk ground only)
  const distTo = (a) => {
    let d = DM.get(a);
    if (d) return d;
    d = new Int32Array(W * H).fill(1e6);
    d[a] = 0;
    const q = [a];
    for (let i = 0; i < q.length; i++) for (const n of nb(q[i])) if (d[n] === 1e6 && hfree(n)) { d[n] = d[q[i]] + 1; q.push(n); }
    DM.set(a, d);
    return d;
  };
  const moves = hunters.map((h) => {
    const pace = Math.max(1, h.variables?.pace || 3);
    let k0 = (h.variables?.due ?? 0) - now;
    if (k0 < 1) k0 = 1;
    return (t) => t >= k0 && (t - k0) % pace === 0;
  });
  const alert = hunters.map((h) => (h.variables?.alert ?? 0) === 1);
  // hunter step: an alert hunter moves along a shortest path to the avatar; the engine's A* treats the
  // other hunters as walls, and its tie-break is not modelled, so each hunter is a SET of possible cells:
  // every successor on a shortest path with or without the other hunters (EXACT=1: one cell). Alert is
  // recomputed at every think like stalk does (d <= SENSE: alert, d > LOSE: lost); a hunter that is not
  // alert walks at random (any free neighbour, or stays).
  const EXACT = process.env.EXACT === "1";
  const alertNow = alert.slice();
  const distAvoid = (a, blockers) => {
    const key = a + "|" + blockers.join(",");
    let d = DM.get(key);
    if (d) return d;
    const bl = new Set(blockers);
    d = new Int32Array(W * H).fill(1e6);
    d[a] = 0;
    const q = [a];
    for (let i = 0; i < q.length; i++) for (const n of nb(q[i])) if (d[n] === 1e6 && free(n) && !bl.has(n)) { d[n] = d[q[i]] + 1; q.push(n); }
    DM.set(key, d);
    return d;
  };
  const step = (hs, a, t, al = alertNow) => {
    const out = hs.slice();
    const toA = distTo(a);
    const ax_ = a % W, ay_ = (a - ax_) / W;
    for (let i = 0; i < out.length; i++) {
      if (!moves[i](t)) continue;
      const nxt = new Set();
      for (const p of out[i]) {
        const md = Math.abs((p % W) - ax_) + Math.abs(Math.floor(p / W) - ay_);
        const isAlert = md <= SENSE ? true : md > LOSE ? false : al[i];
        if (!isAlert) {
          nxt.add(p);
          for (const n of nb(p)) if (n === a || hfree(n)) nxt.add(n);
          continue;
        }
        const blockers = out.filter((o, j) => j !== i && o.size === 1).map((o) => [...o][0]).sort();
        const toB = distAvoid(a, blockers);
        let best = [], be = 1e9;
        for (const n of nb(p)) {
          if (n === a) { best = [n]; break; }
          if (!hfree(n)) continue;
          const sp = toA[n] === toA[p] - 1 || (!blockers.includes(n) && toB[n] === toB[p] - 1);
          if (!sp) continue;
          const e = ((n % W) - ax_) ** 2 + (Math.floor(n / W) - ay_) ** 2;
          if (EXACT) { if (e < be) { be = e; best = [n]; } } else best.push(n);
        }
        if (!best.length) best = [p];
        for (const n of best) nxt.add(n);
      }
      out[i] = nxt;
    }
    return out;
  };
  const hit = (hs, c) => hs.some((set) => set.has(c));
  ctx.stepFn = step;
  const h0 = hunters.map((h) => new Set([h.location[0] + h.location[1] * W]));
  if (hit(h0, first)) return 0;
  const h1 = step(h0, first, 1);
  if (hit(h1, first)) return 0;
  let layer = [[first, h1]];
  let t = 1;
  const seen = new Set([first + ":1"]);
  while (t < ROLL && layer.length) {
    t++;
    const next = [];
    for (const [a, hs] of layer) {
      for (const c of [a, ...nb(a)]) {
        if (c !== a && (!free(c) || danger.has(c))) continue;
        if (hit(hs, c)) continue;
        const key = c + ":" + t;
        if (seen.has(key)) continue;
        const hn = step(hs, c, t);
        if (hit(hn, c)) continue;
        seen.add(key);
        next.push([c, hn]);
      }
    }
    if (!next.length) return t - 1;
    layer = next.length > 300 ? next.slice(0, 300) : next;
  }
  return t;
}

let predicted = null, mism = 0, checks = 0;
function decide() {
  const d = decideRaw();
  if (process.env.CHECK && lastCtx && predicted) {
    const byId = new Map(lastCtx.hunters.map((h) => [h.id, h.location[0] + h.location[1] * lastCtx.W]));
    const act = predicted.ids.map((i) => byId.get(i));
    checks++;
    if (act.some((c, i) => predicted.alert[i] && !predicted.h[i].has(c))) {
      mism++;
      if (mism < 6) console.log("MODEL MISMATCH t", lastCtx.now, "pred", predicted.h.map((s) => [...s].map((c) => [c % lastCtx.W, Math.floor(c / lastCtx.W)])), "actual", act.map((c) => [c % lastCtx.W, Math.floor(c / lastCtx.W)]), "av", [predicted.av % lastCtx.W, Math.floor(predicted.av / lastCtx.W)], "->", [lastCtx.start % lastCtx.W, Math.floor(lastCtx.start / lastCtx.W)], "prev", predicted.prev);
    }
  }
  predicted = null;
  if (!d || !lastCtx || !lastCtx.hunters.length || naive || d[0] !== "move") return d;
  const ctx = lastCtx;
  const { W, start } = ctx;
  // where the avatar ends up: bumping a non-walkable cell (gem, door, wall...) leaves it in place
  const cell = (a) => {
    if (a === 0) return start;
    const n = start + DIRS[a - 1][1] + DIRS[a - 1][2] * W;
    return ctx.free(n) || ctx.walkOn(n) ? n : start;
  };
  const ok = (a) => a === 0 || cell(a) !== start;
  const remember = (a) => {
    if (process.env.CHECK) {
      ctx.predict = true;
      const W = ctx.W;
      const hs0 = ctx.hunters.map((h) => new Set([h.location[0] + h.location[1] * W]));
      ctx.predicted = ctx.stepFn(hs0, cell(a), 1);
      predicted = { ids: ctx.hunters.map((h) => h.id), av: ctx.start, alert: ctx.hunters.map((h) => h.variables.alert === 1), h: ctx.predicted, prev: ctx.hunters.map((h) => [h.location, h.variables.due, h.variables.pace, ctx.now]) };
      ctx.predict = false;
    }
  };
  if (survival(ctx, cell(d[1])) >= ROLL) { remember(d[1]); return d; }
  let best = d[1], bs = -1;
  const dbg = [];
  for (const a of [0, 1, 2, 3, 4]) {
    if (!ok(a) || (a && ctx.danger.has(cell(a)))) continue;
    const sv = survival(ctx, cell(a));
    dbg.push([a, sv]);
    if (sv > bs) { bs = sv; best = a; }
  }
  if (process.env.VETO) console.log("veto t", ctx.now, "wanted", d[1], JSON.stringify(dbg), "->", best);
  remember(best);
  return ["move", best];
}

let lastCtx = null;
function decideRaw() {
  lastCtx = null;
  const s = g.getState();
  const W = s.grid.width, H = s.grid.height;
  const cells = new Map();
  let av = null;
  const foes = [], hunters = [];
  for (const o of s.objects) {
    if (o.location[0] < 0) continue;
    if (o.name === avatarName) { av = o; continue; }
    const k = o.location[0] + o.location[1] * W;
    if (!cells.has(k)) cells.set(k, []);
    cells.get(k).push(o.name);
    if (enemySet.has(o.name)) foes.push(o.location);
    if (hunterSet.has(o.name)) hunters.push(o);
  }
  if (!av) return null;
  const [ax, ay] = av.location;
  const start = ax + ay * W;
  const now = Number(s.globalVariables?._steps?.[0] ?? 0);
  const inb = (x, y) => x >= 0 && y >= 0 && x < W && y < H;
  const danger = new Set();
  for (const [ex, ey] of foes) {
    danger.add(ex + ey * W);
    for (const [, dx, dy] of DIRS) if (inb(ex + dx, ey + dy)) danger.add(ex + dx + (ey + dy) * W);
  }
  const free = (k) => (cells.get(k) ?? []).every((o) => walk.has(o));
  const hasFoe = (k) => (cells.get(k) ?? []).some((o) => killers.has(o));
  // hunters: BFS distance maps over the cells they can walk (ground + the avatar's cell)
  const hd = hunters.map((h) => {
    const d = new Int32Array(W * H).fill(1e6);
    const s0 = h.location[0] + h.location[1] * W;
    d[s0] = 0;
    const q = [s0];
    for (let i = 0; i < q.length; i++) {
      const c = q[i], x = c % W, y = (c - x) / W;
      for (const [, dx, dy] of DIRS) {
        const nx = x + dx, ny = y + dy;
        if (!inb(nx, ny)) continue;
        const n = nx + ny * W;
        if (d[n] <= d[c] + 1) continue;
        if (n !== start && !(cells.get(n) ?? []).every((o) => GROUND.includes(o) || hunterSet.has(o))) continue;
        d[n] = d[c] + 1;
        q.push(n);
      }
    }
    const pace = Math.max(1, h.variables?.pace || 3);
    const k0 = Math.max(1, (h.variables?.due ?? 0) - now);
    return { d, pace, by: (k) => (k < k0 ? 0 : Math.floor((k - k0) / pace) + 1) };
  });
  // can the avatar stand on cell n at its k-th move from now, with `extra` ticks to spare?
  const safeAt = (n, k, extra = 0) => hd.every(({ d, by }) => d[n] > by(k + extra) + MARGIN);
  const hunterMin = (n) => hd.reduce((m, { d, pace }) => Math.min(m, d[n] * pace), 1e6); // ticks
  const hfree = (k) => (cells.get(k) ?? []).every((o) => walk.has(o) || hunterSet.has(o));
  const walkOn = (k) => (cells.get(k) ?? []).some((o) => (cfg.walkOn ?? []).includes(o));
  lastCtx = { W, H, start, hunters, free, hfree, walkOn, danger, now, dist0: (h) => Math.abs(h.location[0] - ax) + Math.abs(h.location[1] - ay) };
  const deg = (n) => {
    const x = n % W, y = (n - x) / W;
    return DIRS.filter(([, dx, dy]) => inb(x + dx, y + dy) && free(x + dx + (y + dy) * W)).length;
  };

  // --search perturbation: a random move into a non-deadly cell (or wait)
  if (perturb > 0) {
    perturb--;
    const opts = [0];
    for (const [a, dx, dy] of DIRS) {
      const n = ax + dx + (ay + dy) * W;
      if (inb(ax + dx, ay + dy) && free(n) && !danger.has(n) && safeAt(n, 1)) opts.push(a);
    }
    return ["move", opts[Math.floor(rand() * opts.length)]];
  }

  // zelda: strike an adjacent enemy (hunters too)
  if (cfg.attack) {
    for (const [a, dx, dy] of DIRS) {
      if (!inb(ax + dx, ay + dy)) continue;
      if (hasFoe(ax + dx + (ay + dy) * W)) return ["attack", a];
    }
    // an enemy two cells away in line: put a flame between us (enemies walking into it die)
    for (const [a, dx, dy] of DIRS) {
      const mid = ax + dx + (ay + dy) * W, far = ax + 2 * dx + (ay + 2 * dy) * W;
      if (!inb(ax + 2 * dx, ay + 2 * dy) || !free(mid)) continue;
      if ((cells.get(mid) ?? []).includes("attack_fire")) continue;
      if (hasFoe(far)) return ["attack", a];
    }
  }
  const goals = new Set(cfg.goals(av));
  // time-aware BFS: returns {step, cell, dist} of the nearest goal (or null)
  const bfs = (avoid, timed, strict = false) => {
    const prev = new Map([[start, -1]]);
    const depth = new Map([[start, 0]]);
    const q = [start];
    for (let i = 0; i < q.length; i++) {
      const c = q[i];
      const x = c % W, y = (c - x) / W;
      const k = depth.get(c) + 1;
      for (const [a, dx, dy] of DIRS) {
        const nx = x + dx, ny = y + dy;
        if (!inb(nx, ny)) continue;
        const n = nx + ny * W;
        if (prev.has(n)) continue;
        if (avoid.has(n)) continue;
        const objs = cells.get(n) ?? [];
        if (objs.some((o) => goals.has(o))) {
          // a goal: bumping it keeps us on c one more tick (walk-on goals put us on n)
          if (timed && !(safeAt(c, k, 1) && safeAt(n, k, 1))) continue;
          if (timed && strict) {
            // we must also get back out: every path cell at depth j is passed again at time 2k - j
            let ok = true;
            for (let cur = c; cur !== start && ok; cur = prev.get(cur)) ok = safeAt(cur, 2 * k - depth.get(cur) + 1);
            if (!ok) continue;
          }
          let cur = c, step = a;
          while (prev.get(cur) !== -1) {
            const p = prev.get(cur);
            const px = p % W;
            step = DIRS.find(([, ddx, ddy]) => px + ddx === cur % W && (p - px) / W + ddy === (cur - (cur % W)) / W)[0];
            cur = p;
          }
          return step;
        }
        if (!free(n)) continue;
        if (timed && !safeAt(n, k)) continue;
        prev.set(n, c);
        depth.set(n, k);
        q.push(n);
      }
    }
    return 0;
  };
  const adjacentFoe = foes.some(([ex, ey]) => Math.abs(ex - ax) + Math.abs(ey - ay) <= 1);
  const step = hd.length ? bfs(danger, true, true) || (STRICT ? 0 : bfs(danger, true)) : bfs(danger, false);
  if (step) { waited = 0; return ["move", step]; }

  if (hd.length) {
    // kite: safely reachable cell maximising the hunters' distance (ties: not a dead end, nearer)
    const prev = new Map([[start, -1]]);
    const depth = new Map([[start, 0]]);
    const q = [start];
    // margin = hunter arrival (ticks) - our arrival; open cells (degree >= 3) are where loops start
    const score = (n, k) => Math.min(hunterMin(n) - k, 40) + 3 * Math.min(deg(n), 3) - k * 0.05;
    let best = start, bestScore = score(start, 0) - (danger.has(start) ? 50 : 0);
    for (let i = 0; i < q.length; i++) {
      const c = q[i];
      const x = c % W, y = (c - x) / W;
      const k = depth.get(c) + 1;
      if (k > 30) continue;
      for (const [, dx, dy] of DIRS) {
        const nx = x + dx, ny = y + dy;
        if (!inb(nx, ny)) continue;
        const n = nx + ny * W;
        if (prev.has(n) || !free(n) || danger.has(n) || !safeAt(n, k)) continue;
        prev.set(n, c);
        depth.set(n, k);
        q.push(n);
        const sc = score(n, k);
        if (sc > bestScore) { bestScore = sc; best = n; }
      }
    }
    if (best !== start) {
      let cur = best;
      while (prev.get(cur) !== start) cur = prev.get(cur);
      const dx = (cur % W) - ax, dy = Math.floor(cur / W) - ay;
      waited = 0;
      return ["move", DIRS.find(([, ddx, ddy]) => ddx === dx && ddy === dy)[0]];
    }
  }
  // armed (zelda): push through, only avoiding the enemies' own cells; it strikes when adjacent
  if (cfg.attack || waited > 12) {
    const occupied = new Set(foes.map(([ex, ey]) => ex + ey * W));
    const bold = bfs(occupied, false);
    if (bold && (!hd.length || safeAt(ax + DIRS[bold - 1][1] + (ay + DIRS[bold - 1][2]) * W, 1))) return ["move", bold];
  }
  if (adjacentFoe || danger.has(start) || (hd.length && !safeAt(start, 1))) {
    // retreat: neighbouring free cell maximising distance to the nearest enemy / hunter
    let best = 0, bestD = -1e9;
    for (const [a, dx, dy] of DIRS) {
      const nx = ax + dx, ny = ay + dy;
      if (!inb(nx, ny) || !free(nx + ny * W) || hasFoe(nx + ny * W)) continue;
      const n = nx + ny * W;
      const d = Math.min(1e3, ...foes.map(([ex, ey]) => Math.abs(ex - nx) + Math.abs(ey - ny))) + Math.min(hunterMin(n), 60);
      if (d > bestD) { bestD = d; best = a; }
    }
    if (best) return ["move", best];
  }
  waited++;
  return ["move", 0]; // wait for the enemies to move away
}

const CH = { spider: "S", hunter: "H", bat: "b", key: "k", door: "D", exit: "E", wall: "W", nest: "N", brood: "B", gem: "g",
  butterfly: "f", cocoon: "c", lava: "L", water: "~", rock: "#", tree: "&", bush: "%", hedge: "$", stone: "@", torch: "|" };
function show() {
  const st = g.getState();
  const grid = Array.from({ length: g.height }, () => Array(g.width).fill("."));
  for (const o of st.objects.sort((a, b) => a.zidx - b.zidx)) {
    if (o.location[0] < 0 || GROUND.includes(o.name)) continue;
    grid[o.location[1]][o.location[0]] = o.name === avatarName ? "A" : (CH[o.name] ?? "#");
  }
  return grid.map((r) => r.join("")).join("\n");
}
function replayDbg(seed, taken) {
  for (const back of [6, 3, 1]) {
    g.seed(seed); g.reset();
    for (const a of taken.slice(0, taken.length - back)) g.step(a);
    console.log(`--- ${back} before death, next actions ${JSON.stringify(taken.slice(taken.length - back))}`);
    console.log(show());
  }
  g.seed(seed); g.reset();
  for (const a of taken) g.step(a);
}

// one episode: returns {res, taken}. In --search mode, deaths rewind and perturb.
function episode(seed) {
  const replay = (prefix) => {
    g.seed(seed);
    g.reset();
    for (const a of prefix) g.step(a);
  };
  replay([]);
  waited = 0;
  perturb = 0;
  rngState = seed * 7919 + 17;
  let taken = [];
  let rewinds = 0;
  const fails = new Map(); // death tick bucket -> count
  while (taken.length < maxSteps) {
    const d = decide();
    let r;
    if (d) {
      taken.push(act(d[0], d[1]));
      r = g.step(act(d[0], d[1]));
    }
    const dead = !d || (r && r.terminated && r.info?.playerResults?.["1"] !== "Win");
    if (r && r.terminated && !dead) return { res: "win", taken };
    if (dead && process.env.DEBUG) {
      replayDbg(seed, taken);
    }
    if (dead) {
      if (!search || rewinds >= 400) return { res: "lose", taken };
      rewinds++;
      const bucket = Math.floor(taken.length / 10);
      const f = (fails.get(bucket) ?? 0) + 1;
      fails.set(bucket, f);
      const back = Math.min(taken.length, 3 + Math.floor(rand() * 4) + 4 * f * f);
      taken = taken.slice(0, taken.length - back);
      replay(taken);
      waited = 0;
      perturb = 1 + Math.floor(rand() * 4);
    }
  }
  return { res: "timeout", taken };
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
    const seed = seed0 + ep;
    const { res, taken } = episode(seed);
    if (res === "win") steps.push(taken.length);
    if (res === "timeout" && process.env.DEBUG) {
      const st = g.getState();
      const a = st.objects.find((o) => o.name === avatarName);
      console.log(`    ep${ep} timeout at ${a?.location} vars=${JSON.stringify(a?.variables)}`);
      console.log(show());
    }
    if (res === "win" && record && !recorded[l]) recorded[l] = { seed, actions: taken.map((x) => (x.length === 1 ? [0, x[0]] : x)) };
    if (res === "win") wins++;
    else if (res === "lose") losses++;
    else timeouts++;
    if (record && search && recorded[l]) break; // one recorded run is enough
  }
  const eps = wins + losses + timeouts;
  total += eps;
  totalWins += wins;
  console.log(`  L${l}: ${g.width}x${g.height} ${naive ? "naive" : search ? "search" : "bot"} win ${wins} lose ${losses} timeout ${timeouts} / ${eps}  steps ${steps.join(",")}`);
}
if (process.env.CHECK) console.log(`model checks ${checks} mismatches ${mism}`);
console.log(`${id}: ${naive ? "naive bot" : search ? "search bot" : "bot"} won ${totalWins}/${total}`);

if (record) {
  // --out=<file>: write there instead (parallel per-level recording, merged afterwards)
  const outArg = process.argv.find((a) => a.startsWith("--out="));
  const out = outArg ? outArg.slice(6) : path.join(here, "../../apps/web/src/games/solutions", `${id}.json`);
  if (onlyLevel !== null) {
    if (!recorded[onlyLevel]) { console.log(`record: no winning episode for level ${onlyLevel} - not written`); process.exit(1); }
    const old = fs.existsSync(out) ? JSON.parse(fs.readFileSync(out, "utf8")) : { format: 2, levels: [] };
    old.levels[onlyLevel] = recorded[onlyLevel];
    for (let i = 0; i < n; i++) old.levels[i] ??= null;
    old.levels.length = n;
    fs.writeFileSync(out, JSON.stringify(old));
    console.log(`record: wrote level ${onlyLevel} to ${out}`);
  } else {
    const missing = [...Array(n).keys()].filter((l) => !recorded[l]);
    if (missing.length) { console.log(`record: no winning episode for levels ${missing.join(",")} - not written`); process.exit(1); }
    fs.mkdirSync(path.dirname(out), { recursive: true });
    fs.writeFileSync(out, JSON.stringify({ format: 2, levels: recorded }));
    console.log(`record: wrote ${out}`);
  }
}
