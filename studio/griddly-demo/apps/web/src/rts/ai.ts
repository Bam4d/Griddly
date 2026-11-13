/**
 * Scripted AI player for Griddly RTS: Frontiers, a port of resources/games/RTS/fow_policy.py
 * (keep the two in step). The GDY holds every rule; this only decides which external actions to
 * issue each tick, at most one per unit, plus camera pans.
 *
 * economy    train workers (up to 6); each mines the nearest free mineral / gem slot, carries ONE
 *            load back to the base and deposits it; with nothing left to mine, workers chop forest
 * expand     a scout walks to the nearest uncaptured relay and captures it
 * build      lab -> barracks -> towers near the base, facing the enemy
 * research   along the strategy's path
 * army       the barracks trains the strategy's units; medics follow and heal; the army gathers at
 *            a rally point and attacks once it is big enough
 *
 * Fog of war: the AI only reads its own player state (`getPlayerState`, filtered by the engine): its
 * own objects, terrain it has explored and enemy objects in sight. It remembers enemy buildings it
 * has seen, guesses the enemy base from the map's point symmetry, and explores with a scout.
 */
import {
  ARMY,
  COSTS,
  DIRS,
  DIR_LIST,
  DISPLAY,
  DOWN,
  FIGHTERS,
  LEFT,
  Planner,
  RIGHT,
  TARGET_PRIORITY,
  TECHS,
  TECH_VARS,
  UNITS,
  BUILDINGS,
  UNIT_NEEDS,
  UP,
  directionTo,
  manhattan,
  minBy,
  neighbours,
  rng,
  sameCell,
  step,
} from "./world";
import type { Cell, Dir, Obj, PlannedAction, WorldView } from "./world";

export type Strategy = { name: string; research: number[]; army: [string, number][]; towers: number };

export const STRATEGIES: Record<string, Strategy> = {
  GUNLINE: { name: "GUNLINE", research: [1, 2, 4, 3], army: [["marine", 3], ["combat", 2], ["medic", 1]], towers: 1 },
  ARMOUR: { name: "ARMOUR", research: [1, 3, 5, 6], army: [["tank", 2], ["combat", 3], ["medic", 1]], towers: 2 },
  RUSH: { name: "RUSH", research: [2, 1, 3], army: [["combat", 4], ["scout", 1]], towers: 0 },
};

export const TARGET_WORKERS = 6;
const CAMERA_PANS_PER_TICK = 2;

export type AiOptions = { strategy?: Strategy; attackSize?: number; seed?: number };

export class RTSPolicy {
  readonly strategy: Strategy;
  readonly attackSize: number;
  offensive = false;
  phase = "ECONOMY";
  private counts = new Map<string, number>();
  private events: [number, string][] = [];
  private lastTech = new Map<string, number>();
  private sites = new Map<string, Cell | null>();
  private rally: [Cell, Cell] | null = null;
  /** enemy buildings seen (cell key -> [cell, name]), until their cell is seen without them */
  private knownEnemy = new Map<number, [Cell, string]>();
  private exploreGoals = new Map<string, Cell | null>();
  private random: () => number;

  constructor(
    readonly pid: number,
    readonly enemy: number,
    opts: AiOptions = {}
  ) {
    this.strategy = opts.strategy ?? (pid === 1 ? STRATEGIES.GUNLINE! : STRATEGIES.ARMOUR!);
    this.attackSize = opts.attackSize ?? 7;
    this.random = rng((opts.seed ?? 0) * 1000 + pid);
  }

  // ---------------------------------------------------------------- helpers
  private emit(actions: PlannedAction[], name: string, pos: Cell, id: number, label: string) {
    actions.push([name, pos[0], pos[1], id]);
    this.counts.set(label, (this.counts.get(label) ?? 0) + 1);
  }

  /** One step towards any goal cell; sidestep randomly when the way is jammed by units. */
  private walk(plan: Planner, actions: PlannedAction[], unit: Obj, goals: Cell[], label: string): boolean {
    const pos = unit.pos;
    let s = plan.bfs(pos, goals);
    if (s === null) s = plan.bfs(pos, goals, true);
    if (s === null || s.dir === null) return false;
    let d: Dir = s.dir;
    let nxt = step(pos, d);
    if (!plan.free(nxt)) {
      const options = DIR_LIST.filter((k) => plan.free(step(pos, k)));
      if (options.length === 0 || this.random() < 0.5) return false;
      d = options[Math.floor(this.random() * options.length)]!;
      nxt = step(pos, d);
    }
    plan.move(pos, nxt);
    this.emit(actions, "move", pos, d, label);
    unit.pos = nxt;
    return true;
  }

  /** A free tile dmin..dmax steps from `base`, not touching `avoid`, closest to `towards`. */
  private site(world: WorldView, plan: Planner, base: Cell, towards: Cell, dmin: number, dmax: number, avoid: Set<string>): Cell | null {
    let best: { score: number[]; c: Cell } | null = null;
    for (let y = Math.max(0, base[1] - dmax); y < Math.min(world.height, base[1] + dmax + 1); y++) {
      for (let x = Math.max(0, base[0] - dmax); x < Math.min(world.width, base[0] + dmax + 1); x++) {
        const c: Cell = [x, y];
        const d = manhattan(c, base);
        if (d < dmin || d > dmax || !plan.free(c) || plan.isReserved(c)) continue;
        if (world.get(c) !== undefined) continue;
        if (neighbours(c).some((n) => avoid.has(world.get(n)?.name ?? ""))) continue;
        if (neighbours(c).filter((n) => plan.free(n)).length < 3) continue;
        const score = [manhattan(c, towards), x, y];
        if (best === null || cmp(score, best.score) < 0) best = { score, c };
      }
    }
    return best ? best.c : null;
  }

  /** Send the nearest idle worker next to `site` and build there. */
  private construct(world: WorldView, plan: Planner, actions: PlannedAction[], workers: Obj[], site: Cell | null, building: string, res: number): [Obj | null, number] {
    const free = workers.filter((w) => !w.vars.carrying);
    if (!site || free.length === 0) return [null, res];
    const builder = minBy(free, (w) => manhattan(w.pos, site))!;
    const d = directionTo(builder.pos, site);
    if (d !== null) {
      if (res >= COSTS[building]! && world.get(site) === undefined) {
        this.emit(actions, `build_${building}`, builder.pos, d, `BUILD ${building.toUpperCase()}`);
        plan.spawn(site);
        res -= COSTS[building]!;
      }
      plan.reserve(builder.pos);
    } else {
      const goals = neighbours(site).filter((n) => plan.free(n) || sameCell(n, builder.pos));
      this.walk(plan, actions, builder, goals, `walk to ${building} site`);
    }
    return [builder, res];
  }

  private nextResearch(world: WorldView): number | null {
    for (const tid of this.strategy.research) if (world.tech(TECHS[tid]!.variable, this.pid) !== 2) return tid;
    return null;
  }

  /** The unit of the strategy's mix the army is most short of, among those researched. */
  private unitChoice(world: WorldView, army: Obj[]): string {
    const counts = new Map<string, number>();
    for (const u of army) counts.set(u.name, (counts.get(u.name) ?? 0) + 1);
    let best: [number, string] | null = null;
    for (const [unit, share] of this.strategy.army) {
      const needs = UNIT_NEEDS[unit];
      if (needs && world.tech(needs, this.pid) !== 2) continue;
      const ratio = (counts.get(unit) ?? 0) / share;
      if (best === null || ratio < best[0]) best = [ratio, unit];
    }
    return best ? best[1] : "combat";
  }

  private remember(world: WorldView) {
    for (const o of world.objects) {
      if (o.player === this.enemy && BUILDINGS.includes(o.name)) this.knownEnemy.set(world.key(o.pos), [o.pos, o.name]);
    }
    for (const [k, [pos]] of this.knownEnemy) {
      const o = world.get(pos);
      if (world.visible(pos) && (!o || o.player !== this.enemy)) this.knownEnemy.delete(k);
    }
  }

  /** The enemy base: seen, remembered, or the mirror of ours (the maps are point-symmetric). */
  private enemyBaseGuess(world: WorldView, base: Cell): Cell {
    const seen = world.of("base", this.enemy)[0];
    if (seen) return seen.pos;
    for (const [pos, name] of this.knownEnemy.values()) if (name === "base") return pos;
    return [world.width - 1 - base[0], world.height - 1 - base[1]];
  }

  /** Walk towards the nearest reachable unexplored cell (sampled every 3 cells). */
  private explore(world: WorldView, plan: Planner, actions: PlannedAction[], u: Obj, label: string): boolean {
    let goal = this.exploreGoals.get(label) ?? null;
    if (goal === null || !world.unseen(goal) || manhattan(u.pos, goal) <= 2 || this.random() < 0.02) {
      const cells: Cell[] = [];
      for (let y = 1; y < world.height - 1; y += 3) for (let x = 1; x < world.width - 1; x += 3) if (world.unseen([x, y])) cells.push([x, y]);
      const s = cells.length ? plan.bfs(u.pos, cells, true, 8000) : null;
      goal = s ? s.goal : null;
      this.exploreGoals.set(label, goal);
    }
    return goal !== null && this.walk(plan, actions, u, [goal, ...neighbours(goal)], label);
  }

  // ----------------------------------------------------------------- policy
  /** The actions for this tick and a short caption of what the AI is doing. */
  act(world: WorldView): { actions: PlannedAction[]; caption: string[] } {
    this.counts = new Map();
    const plan = new Planner(world);
    const actions: PlannedAction[] = [];
    const pid = this.pid;
    let res = world.resources(pid);
    const base = world.of("base", pid)[0]?.pos ?? null;
    this.remember(world);
    // the enemy base as far as we know: seen, remembered, or guessed
    const enemyBase = base ? this.enemyBaseGuess(world, base) : null;
    // positions are mutated as units are planned, so work on copies
    const mine = world.objects.filter((o) => o.player === pid).map((o) => ({ ...o }));
    const workers = mine.filter((o) => o.name === "worker");
    const army = mine.filter((o) => ARMY.includes(o.name));
    const barracks = mine.filter((o) => o.name === "barracks");
    const labs = mine.filter((o) => o.name === "lab");
    const towers = mine.filter((o) => o.name === "tower");
    const enemies = world.objects.filter((o) => o.player === this.enemy && o.name in TARGET_PRIORITY);
    const enemyAt = new Map<number, Obj>(enemies.map((o) => [world.key(o.pos), o]));
    // remembered buildings out of sight right now are still worth marching to
    const remembered: Obj[] = [...this.knownEnemy.entries()]
      .filter(([k, [, name]]) => !enemyAt.has(k) && name in TARGET_PRIORITY)
      .map(([k, [pos, name]]) => ({ id: -1 - k, name, pos, player: this.enemy, vars: {} }));
    const resources = [...world.of("minerals"), ...world.of("gems")];
    const relays = world.objects.filter((o) => o.name === "relay" || (o.name === "relay_owned" && o.player !== pid));
    this.trackResearch(world);
    const nextTech = this.nextResearch(world);
    const researching = TECH_VARS.some((v) => world.tech(v, pid) === 1);
    if (base === null) {
      this.phase = "DEFEATED";
      return { actions, caption: this.caption(world, res, workers, army) };
    }
    const threat = enemies.filter((e) => UNITS.includes(e.name) && manhattan(e.pos, base) <= 8);

    // -------------------------------------------------------- base: workers
    if (workers.length < TARGET_WORKERS && res >= COSTS.worker! && (workers.length < 3 || labs.length > 0)) {
      for (const d of [DOWN, RIGHT, UP, LEFT]) {
        const c = step(base, d);
        if (plan.free(c)) {
          this.emit(actions, "build_worker", base, d, "train worker");
          plan.spawn(c);
          res -= COSTS.worker!;
          break;
        }
      }
    }

    // ------------------------------------------------------------ buildings
    const toward: Cell = enemyBase ?? [Math.floor(world.width / 2), Math.floor(world.height / 2)];
    const avoid = new Set(["minerals", "gems", "base", "barracks", "lab", "tower"]);
    const builders = new Set<number>();
    let wanted: [string, Cell | null] | null = null;
    if (labs.length === 0 && workers.length >= 3) {
      wanted = ["lab", this.site(world, plan, base, [base[0] * 2 - toward[0], base[1] * 2 - toward[1]], 2, 4, avoid)];
    } else if (labs.length > 0 && barracks.length === 0 && workers.length >= 4) {
      wanted = ["barracks", this.site(world, plan, base, toward, 3, 5, avoid)];
    } else if (barracks.length > 0 && towers.length < this.strategy.towers && (threat.length > 0 || army.length >= 3)) {
      wanted = ["tower", this.site(world, plan, base, toward, 4, 6, avoid)];
    }
    if (wanted) {
      const kind = wanted[0];
      let s = this.sites.get(kind) ?? null;
      const occupant = s ? world.get(s) : undefined;
      if (s === null || (occupant !== undefined && !UNITS.includes(occupant.name))) {
        s = wanted[1];
        this.sites.set(kind, s);
      }
      wanted = [kind, s];
      if (s) plan.reserve(s);
      if (s && res >= COSTS[kind]! - 3) {
        let builder: Obj | null;
        [builder, res] = this.construct(world, plan, actions, workers, s, kind, res);
        if (builder) builders.add(builder.id);
      }
    }

    // ------------------------------------------------------------- research
    if (labs.length > 0 && nextTech !== null && !researching) {
      const t = TECHS[nextTech]!;
      if (res >= t.cost && (t.needs === null || world.tech(t.needs, pid) === 2)) {
        this.emit(actions, "research", labs[0]!.pos, nextTech, `RESEARCH ${t.name.toUpperCase()}`);
        res -= t.cost;
      }
    }

    // ----------------------------------------------------------------- army
    const unit = this.unitChoice(world, army);
    const keep = threat.length > 0 ? 0 : nextTech !== null && labs.length > 0 && !researching ? TECHS[nextTech]!.cost : 0;
    for (const b of barracks) {
      if (res - keep < COSTS[unit]!) break;
      const spots = DIR_LIST.filter((k) => plan.free(step(b.pos, k))).map((k) => ({ k, d: manhattan(step(b.pos, k), toward) }));
      const best = minBy(spots, (s) => [s.d, s.k]);
      if (best) {
        this.emit(actions, `build_${unit}`, b.pos, best.k, `train ${DISPLAY[unit]}`);
        plan.spawn(step(b.pos, best.k));
        res -= COSTS[unit]!;
      }
    }
    // a scout grabs relays and explores the fog (relays and minerals out of sight are unknown)
    const unexplored = world.visibility !== null && world.unseenCount() > world.visibility.length / 3;
    if (barracks.length > 0 && !army.some((u) => u.name === "scout") && (relays.length > 0 || unexplored || resources.length === 0) && res >= COSTS.scout! + keep && world.tick < 900) {
      const b = barracks[0]!.pos;
      for (const d of [RIGHT, DOWN, LEFT, UP]) {
        const c = step(b, d);
        if (plan.free(c)) {
          this.emit(actions, "build_scout", b, d, "train scout");
          plan.spawn(c);
          res -= COSTS.scout!;
          break;
        }
      }
    }

    // -------------------------------------------------------------- workers
    this.workers(world, plan, actions, workers, builders, base, resources, enemyAt, threat);

    // ----------------------------------------------------------------- army
    const fighters = army.filter((u) => FIGHTERS.includes(u.name));
    // attack with a full army, with whatever we have once nothing known is left to mine, or when
    // the barracks is boxed in by idle units
    const boxedIn = barracks.length > 0 && barracks.every((b) => neighbours(b.pos).every((n) => !plan.free(n)));
    if (!this.offensive && (fighters.length >= this.attackSize || (resources.length === 0 && fighters.length > 0 && (res < 5 || boxedIn)) || (boxedIn && fighters.length >= 3))) {
      this.offensive = true;
    }
    if (this.offensive && fighters.length < 2 && enemyBase && threat.length === 0) this.offensive = false;
    const sgn = (a: number, b: number) => (a > b ? 6 : a < b ? -6 : 0);
    let rally: Cell = [base[0] + sgn(toward[0], base[0]), base[1] + sgn(toward[1], base[1])];
    rally = [Math.min(Math.max(rally[0], 1), world.width - 2), Math.min(Math.max(rally[1], 1), world.height - 2)];
    rally = this.rallyPoint(world, base, rally);
    const sorted = [...army].sort((a, b) => manhattan(a.pos, toward) - manhattan(b.pos, toward));
    for (const u of sorted) {
      if (u.name === "scout") this.scout(world, plan, actions, u, relays, enemyAt);
      else if (u.name === "medic") this.medic(world, plan, actions, u, army, rally);
      else this.fight(world, plan, actions, u, enemies, enemyAt, enemyBase, rally, threat, remembered);
    }

    // --------------------------------------------------------------- camera
    const focus = this.cameraFocus(base, workers, fighters, enemyBase);
    let cam = world.camera(pid);
    for (let i = 0; i < CAMERA_PANS_PER_TICK; i++) {
      const dx = focus[0] - cam[0];
      const dy = focus[1] - cam[1];
      if (Math.abs(dx) <= 1 && Math.abs(dy) <= 1) break;
      const d: Dir = Math.abs(dx) >= Math.abs(dy) ? (dx > 0 ? RIGHT : LEFT) : dy > 0 ? DOWN : UP;
      actions.push(["move_camera", base[0], base[1], d]);
      cam = [cam[0] + 2 * DIRS[d][0], cam[1] + 2 * DIRS[d][1]];
    }

    this.phase = this.offensive ? "ATTACK" : threat.length ? "DEFEND" : barracks.length ? "ARMY" : labs.length ? "TECH" : "ECONOMY";
    return { actions, caption: this.caption(world, world.resources(pid), workers, army) };
  }

  // ------------------------------------------------------------------ roles
  private workers(world: WorldView, plan: Planner, actions: PlannedAction[], workers: Obj[], builders: Set<number>, base: Cell, resources: Obj[], enemyAt: Map<number, Obj>, threat: Obj[]) {
    const resAt = new Map<number, Obj>(resources.map((r) => [world.key(r.pos), r]));
    const eb = this.enemyBaseGuess(world, base);
    // mine what is closer to us than to the enemy (or anything once ours is gone)
    let targets = resources.filter((r) => eb === null || manhattan(r.pos, base) <= manhattan(r.pos, eb) + 4).map((r) => r.pos);
    if (targets.length === 0) targets = resources.map((r) => r.pos);
    const targetKeys = new Set(targets.map((t) => world.key(t)));
    const drop = neighbours(base);
    const distToTarget = (w: Obj) => (targets.length ? Math.min(...targets.map((p) => manhattan(w.pos, p))) : 0);
    const order = [...workers].sort((a, b) => (a.vars.carrying ? 0 : 1) - (b.vars.carrying ? 0 : 1) || distToTarget(a) - distToTarget(b));
    for (const w of order) {
      if (builders.has(w.id)) continue;
      const pos = w.pos;
      const adjacent = neighbours(pos).filter((n) => world.inside(n) && enemyAt.has(world.key(n)));
      if (adjacent.length) {
        const t = minBy(adjacent, (c) => TARGET_PRIORITY[enemyAt.get(world.key(c))!.name]!)!;
        this.emit(actions, "attack", pos, directionTo(pos, t)!, "worker defends");
        continue;
      }
      if (w.vars.carrying) {
        if (manhattan(pos, base) === 1) this.emit(actions, "gather", pos, directionTo(pos, base)!, "deposit");
        else this.walk(plan, actions, w, drop, "carry");
        continue;
      }
      // repair damaged buildings when there is a threat
      const damaged = neighbours(pos).filter((n) => {
        const o = world.get(n);
        return o && o.player === this.pid && ["base", "barracks", "tower", "lab", "wall"].includes(o.name) && (o.vars.health ?? 0) < (o.vars.max_health ?? 0);
      });
      if (damaged.length && threat.length) {
        this.emit(actions, "repair", pos, directionTo(pos, damaged[0]!)!, "repair");
        continue;
      }
      const near = neighbours(pos).filter((n) => world.inside(n) && targetKeys.has(world.key(n)));
      if (near.length) {
        const gem = near.filter((n) => resAt.get(world.key(n))?.name === "gems");
        plan.reserve(pos);
        this.emit(actions, "gather", pos, directionTo(pos, (gem[0] ?? near[0])!)!, "mine");
        continue;
      }
      const slots = slotsAround(plan, targets);
      const s = slots.length ? plan.bfs(pos, slots) : null;
      if (s !== null) {
        plan.reserve(s.goal);
        this.walk(plan, actions, w, [s.goal], "to minerals");
        continue;
      }
      // nothing to mine: chop the nearest forest for wood
      const trees = neighbours(pos).filter((n) => world.get(n)?.name === "forest");
      if (trees.length) {
        this.emit(actions, "gather", pos, directionTo(pos, trees[0]!)!, "chop wood");
        continue;
      }
      const forest = world.of("forest").filter((o) => manhattan(o.pos, base) <= 12).map((o) => o.pos);
      const fslots = slotsAround(plan, forest);
      const f = fslots.length ? plan.bfs(pos, fslots) : null;
      if (f !== null) {
        plan.reserve(f.goal);
        this.walk(plan, actions, w, [f.goal], "to forest");
      } else if (resources.length === 0 && w === order[0]) {
        // nothing known to mine: one worker looks for more
        this.explore(world, plan, actions, w, "worker explores");
      }
    }
  }

  private scout(world: WorldView, plan: Planner, actions: PlannedAction[], u: Obj, relays: Obj[], enemyAt: Map<number, Obj>) {
    const pos = u.pos;
    if (relays.length === 0) {
      // no relay known: explore the fog (and find the enemy)
      const enemyAdjacent = neighbours(pos).some((n) => world.inside(n) && enemyAt.has(world.key(n)));
      if (!enemyAdjacent && this.explore(world, plan, actions, u, "scout explores")) return;
      this.fight(world, plan, actions, u, [...enemyAt.values()], enemyAt, null, pos, []);
      return;
    }
    const target = minBy(relays, (r) => manhattan(r.pos, pos))!;
    if (manhattan(target.pos, pos) === 1) {
      // the GDY's capture trigger does the rest while the scout stands here
      plan.reserve(pos);
      this.counts.set("capture relay", (this.counts.get("capture relay") ?? 0) + 1);
      return;
    }
    this.walk(plan, actions, u, neighbours(target.pos), "to relay");
  }

  private medic(world: WorldView, plan: Planner, actions: PlannedAction[], u: Obj, army: Obj[], rally: Cell) {
    const pos = u.pos;
    const hurt = neighbours(pos).filter((n) => {
      const o = world.get(n);
      return o && o.player === this.pid && UNITS.includes(o.name) && (o.vars.health ?? 0) < (o.vars.max_health ?? 0);
    });
    if (hurt.length) {
      this.emit(actions, "heal", pos, directionTo(pos, hurt[0]!)!, "heal");
      return;
    }
    const wounded = army.filter((a) => a !== u && (a.vars.health ?? 0) < (a.vars.max_health ?? 0));
    const friends = wounded.length ? wounded : army.filter((a) => a !== u && FIGHTERS.includes(a.name));
    if (friends.length) {
      const f = minBy(friends, (a) => manhattan(a.pos, pos))!;
      if (manhattan(f.pos, pos) > 1) this.walk(plan, actions, u, neighbours(f.pos), "follow army");
    } else if (manhattan(pos, rally) > 2) {
      this.walk(plan, actions, u, [rally, ...neighbours(rally)], "rally");
    } else {
      this.makeRoom(world, plan, actions, u);
    }
  }

  private fight(world: WorldView, plan: Planner, actions: PlannedAction[], u: Obj, enemies: Obj[], enemyAt: Map<number, Obj>, enemyBase: Cell | null, rally: Cell, threat: Obj[], remembered: Obj[] = []) {
    const pos = u.pos;
    const kind = u.name;
    const enemyOn = (c: Cell) => (world.inside(c) ? enemyAt.get(world.key(c)) : undefined);
    const prio = (c: Cell) => TARGET_PRIORITY[enemyOn(c)!.name]!;
    if (kind === "tank" && u.vars.cooldown === 1) {
      const adj = neighbours(pos).filter((n) => enemyOn(n));
      if (adj.length) {
        const t = minBy(adj, prio)!;
        this.emit(actions, "attack", pos, directionTo(pos, t)!, `attack ${DISPLAY[enemyOn(t)!.name]}`);
      }
      plan.reserve(pos);
      return;
    }
    if (kind === "marine") {
      const shots = DIR_LIST.map((k) => ({ c: [pos[0] + 2 * DIRS[k][0], pos[1] + 2 * DIRS[k][1]] as Cell, k })).filter((s) => enemyOn(s.c));
      if (shots.length) {
        const s = minBy(shots, (t) => prio(t.c))!;
        this.emit(actions, "shoot", pos, s.k, `shoot ${DISPLAY[enemyOn(s.c)!.name]}`);
        return;
      }
    }
    const adj = neighbours(pos).filter((n) => enemyOn(n));
    if (adj.length) {
      const t = minBy(adj, prio)!;
      this.emit(actions, "attack", pos, directionTo(pos, t)!, `attack ${DISPLAY[enemyOn(t)!.name]}`);
      return;
    }
    let close = enemies.filter((e) => UNITS.includes(e.name) && manhattan(e.pos, pos) <= 5);
    if (threat.length && !this.offensive && close.length === 0) close = threat;
    if (this.offensive || close.length) {
      let tgt: Obj;
      let label: string;
      if (close.length) {
        tgt = minBy(close, (e) => [manhattan(e.pos, pos), TARGET_PRIORITY[e.name]!])!;
        label = "engage";
      } else {
        const known = [...enemies, ...remembered];
        const nearTower = known.filter((e) => e.name === "tower" && manhattan(e.pos, pos) <= 6);
        const bases = known.filter((e) => e.name === "base");
        const pool = nearTower.length ? nearTower : bases.length ? bases : known;
        if (pool.length === 0) {
          // nothing known: head for where the enemy base should be
          if (enemyBase) this.walk(plan, actions, u, [enemyBase, ...neighbours(enemyBase)], "search");
          return;
        }
        tgt = minBy(pool, (e) => manhattan(e.pos, pos))!;
        label = "march";
      }
      if (kind === "marine") {
        const goals = DIR_LIST.map((k) => [tgt.pos[0] + 2 * DIRS[k][0], tgt.pos[1] + 2 * DIRS[k][1]] as Cell).filter((g) => world.inside(g));
        if (this.walk(plan, actions, u, goals, label)) return;
      }
      this.walk(plan, actions, u, neighbours(tgt.pos), label);
    } else if (manhattan(pos, rally) > 2) {
      this.walk(plan, actions, u, [rally, ...neighbours(rally)], "rally");
    } else {
      this.makeRoom(world, plan, actions, u);
    }
  }

  /** Idle units step off the cells next to friendly production buildings. */
  private makeRoom(world: WorldView, plan: Planner, actions: PlannedAction[], u: Obj) {
    const crowds = (c: Cell) =>
      neighbours(c).some((n) => {
        const o = world.get(n);
        return o && o.player === this.pid && (o.name === "barracks" || o.name === "base");
      });
    if (crowds(u.pos)) {
      const away = neighbours(u.pos).filter((n) => plan.free(n) && !crowds(n));
      if (away.length) this.walk(plan, actions, u, away, "make room");
    }
  }

  /** The open cell reachable from the base that is closest to `want` (computed once). */
  private rallyPoint(world: WorldView, base: Cell, want: Cell): Cell {
    if (this.rally && sameCell(this.rally[0], want)) return this.rally[1];
    const blocking = new Set(["forest", "rock", "water", "minerals", "gems", "relay"]);
    const seen = new Set<number>([world.key(base)]);
    const queue: Cell[] = [base];
    let head = 0;
    let best: [number, Cell] = [manhattan(base, want), base];
    while (head < queue.length && seen.size < 6000) {
      const cur = queue[head++]!;
      for (const n of neighbours(cur)) {
        if (!world.inside(n) || seen.has(world.key(n))) continue;
        const o = world.get(n);
        if (o && blocking.has(o.name)) continue;
        seen.add(world.key(n));
        queue.push(n);
        if (o === undefined && manhattan(n, want) < best[0]) best = [manhattan(n, want), n];
      }
    }
    this.rally = [want, best[1]];
    return best[1];
  }

  // --------------------------------------------------------------- captions
  private cameraFocus(base: Cell, workers: Obj[], fighters: Obj[], enemyBase: Cell | null): Cell {
    if (this.offensive && fighters.length && enemyBase) return minBy(fighters, (u) => manhattan(u.pos, enemyBase))!.pos;
    const units = fighters.length ? fighters : workers;
    if (units.length === 0) return base;
    const xs = [...units.map((u) => u.pos[0]), base[0]];
    const ys = [...units.map((u) => u.pos[1]), base[1]];
    return [Math.round(xs.reduce((a, b) => a + b) / xs.length), Math.round(ys.reduce((a, b) => a + b) / ys.length)];
  }

  private trackResearch(world: WorldView) {
    for (const t of Object.values(TECHS)) {
      const state = world.tech(t.variable, this.pid);
      if ((this.lastTech.get(t.variable) ?? 0) !== state && state === 2) this.events.push([world.tick, `${t.name.toUpperCase()} DONE`]);
      this.lastTech.set(t.variable, state);
    }
    this.events = this.events.filter((e) => world.tick - e[0] < 6);
  }

  private caption(world: WorldView, res: number, workers: Obj[], army: Obj[]): string[] {
    const parts = this.events.map((e) => e[1]);
    const keys = [...this.counts.keys()].sort((a, b) => {
      const ua = a === a.toUpperCase() ? 0 : 1;
      const ub = b === b.toUpperCase() ? 0 : 1;
      return ua - ub || this.counts.get(b)! - this.counts.get(a)!;
    });
    parts.push(...keys.map((k) => (this.counts.get(k)! > 1 ? `${k} x${this.counts.get(k)}` : k)));
    const counts = new Map<string, number>();
    for (const u of army) counts.set(DISPLAY[u.name]!, (counts.get(DISPLAY[u.name]!) ?? 0) + 1);
    const armyTxt = [...counts.entries()].sort().map(([k, n]) => `${n} ${k}`).join(" ") || "none";
    const relays = world.of("relay_owned", this.pid).length;
    const tech = this.strategy.research
      .map((t) => {
        const s = world.tech(TECHS[t]!.variable, this.pid);
        return TECHS[t]!.name + (s === 2 ? "+" : s === 1 ? "..." : "");
      })
      .join(" ");
    return [
      `${this.strategy.name} ${this.phase}  res=${res} relays=${relays}`,
      `workers=${workers.length} army: ${armyTxt}`,
      `tech: ${tech}`,
      parts.length ? parts.slice(0, 3).join(", ") : "(waiting)",
    ];
  }
}

const cmp = (a: number[], b: number[]) => {
  for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return a[i]! - b[i]!;
  return 0;
};

/** Free, unreserved cells next to any of `cells`. */
function slotsAround(plan: Planner, cells: Cell[]): Cell[] {
  const out: Cell[] = [];
  const seen = new Set<number>();
  for (const p of cells) {
    for (const n of neighbours(p)) {
      if (!plan.free(n) || plan.isReserved(n)) continue;
      const k = plan.world.key(n);
      if (!seen.has(k)) {
        seen.add(k);
        out.push(n);
      }
    }
  }
  return out;
}
