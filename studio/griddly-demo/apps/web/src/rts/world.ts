/**
 * Shared game-state helpers for Griddly RTS: Frontiers (GriddlyRTS-FoW.yaml).
 *
 * The GDY holds every rule. The players (the human UI's order queue in orders.ts and the scripted
 * AI in ai.ts) only read this snapshot and issue external actions, one per unit per tick.
 */

export type Cell = readonly [number, number];
export type Dir = 1 | 2 | 3 | 4;
export const LEFT: Dir = 1;
export const UP: Dir = 2;
export const RIGHT: Dir = 3;
export const DOWN: Dir = 4;
export const DIRS: Record<Dir, Cell> = { 1: [-1, 0], 2: [0, -1], 3: [1, 0], 4: [0, 1] };
export const DIR_LIST: Dir[] = [LEFT, UP, RIGHT, DOWN];

export const COSTS: Record<string, number> = {
  worker: 5,
  scout: 4,
  combat: 5,
  marine: 6,
  medic: 6,
  tank: 12,
  barracks: 15,
  lab: 10,
  tower: 12,
  wall: 2,
};
export const UNITS = ["worker", "scout", "combat", "marine", "medic", "tank"];
export const ARMY = ["scout", "combat", "marine", "medic", "tank"];
export const FIGHTERS = ["combat", "marine", "tank"];
export const BUILDINGS = ["base", "barracks", "lab", "tower", "wall", "relay_owned"];
export const STATIC = new Set([
  "forest",
  "rock",
  "water",
  "minerals",
  "gems",
  "relay",
  "base",
  "barracks",
  "lab",
  "tower",
  "wall",
  "relay_owned",
]);
export const DISPLAY: Record<string, string> = {
  combat: "trooper",
  marine: "gunner",
  tank: "tank",
  worker: "worker",
  scout: "scout",
  medic: "medic",
  base: "base",
  barracks: "barracks",
  lab: "lab",
  tower: "tower",
  wall: "wall",
  relay_owned: "relay",
  relay: "neutral relay",
  minerals: "minerals",
  gems: "gems",
  forest: "forest",
  rock: "rock",
  water: "water",
  bridge: "bridge",
};

/** Research tree: tech id -> global variable, name, cost, prerequisite variable. */
export const TECHS: Record<number, { variable: string; name: string; cost: number; needs: string | null }> = {
  1: { variable: "res_mining", name: "Mining", cost: 8, needs: null },
  2: { variable: "res_weapons", name: "Weapons", cost: 10, needs: null },
  3: { variable: "res_armor", name: "Armour", cost: 10, needs: null },
  4: { variable: "res_gunnery", name: "Gunnery", cost: 12, needs: "res_weapons" },
  5: { variable: "res_vehicles", name: "Vehicles", cost: 12, needs: "res_armor" },
  6: { variable: "res_fortify", name: "Fortify", cost: 10, needs: "res_armor" },
};
export const TECH_VARS = Object.values(TECHS).map((t) => t.variable);
export const UNIT_NEEDS: Record<string, string> = { marine: "res_gunnery", tank: "res_vehicles" };
export const TARGET_PRIORITY: Record<string, number> = {
  marine: 0,
  medic: 1,
  combat: 2,
  tank: 3,
  scout: 4,
  worker: 5,
  tower: 6,
  barracks: 7,
  lab: 8,
  wall: 10,
  base: 11,
};

export const manhattan = (a: Cell, b: Cell) => Math.abs(a[0] - b[0]) + Math.abs(a[1] - b[1]);
export const neighbours = (c: Cell): Cell[] => DIR_LIST.map((d) => [c[0] + DIRS[d][0], c[1] + DIRS[d][1]] as Cell);
export const step = (c: Cell, d: Dir): Cell => [c[0] + DIRS[d][0], c[1] + DIRS[d][1]];
export const directionTo = (src: Cell, dst: Cell): Dir | null => {
  const dx = dst[0] - src[0];
  const dy = dst[1] - src[1];
  for (const d of DIR_LIST) if (DIRS[d][0] === dx && DIRS[d][1] === dy) return d;
  return null;
};
export const sameCell = (a: Cell, b: Cell) => a[0] === b[0] && a[1] === b[1];

/** The first element with the smallest key (like Python's min(key=...)). */
export function minBy<T>(items: Iterable<T>, key: (t: T) => number | number[]): T | undefined {
  let best: T | undefined;
  let bestKey: number | number[] | undefined;
  for (const it of items) {
    const k = key(it);
    if (bestKey === undefined || compareKeys(k, bestKey) < 0) {
      best = it;
      bestKey = k;
    }
  }
  return best;
}
const compareKeys = (a: number | number[], b: number | number[]): number => {
  const x = Array.isArray(a) ? a : [a];
  const y = Array.isArray(b) ? b : [b];
  for (let i = 0; i < Math.min(x.length, y.length); i++) {
    const d = (x[i] ?? 0) - (y[i] ?? 0);
    if (d !== 0) return d;
  }
  return x.length - y.length;
};

/** Small deterministic PRNG (mulberry32) so AI runs replay for a seed. */
export function rng(seed: number): () => number {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export type Obj = {
  id: number;
  name: string;
  pos: Cell;
  player: number;
  vars: Record<string, number>;
};

/**
 * One player's state from the engine, `Griddly.getPlayerState(playerId)`: filtered by the GDY's
 * Environment.Player.FogOfWar (own objects, explored terrain, enemies only while in sight, only that
 * player's per-player global variables), plus the player's fog cells.
 */
export type RawState = {
  gameTicks: number;
  objects: { id: number; name: string; location: [number, number]; playerId: number; variables?: Record<string, number> }[];
  /** variable name -> values indexed by player id (index 0 for shared variables) */
  globalVariables?: Record<string, number[]>;
  /** fog of war per cell, row major: 0 never seen, 1 explored, 2 visible; null without fog */
  visibility?: Uint8Array | null;
};

export const UNSEEN = 0;
export const EXPLORED = 1;
export const VISIBLE = 2;

/** What one player knows for one tick (never more than the engine's player state). */
export class WorldView {
  readonly tick: number;
  readonly objects: Obj[] = [];
  readonly at = new Map<number, Obj>();
  readonly byName = new Map<string, Obj[]>();
  readonly byId = new Map<number, Obj>();

  readonly globals: Record<string, number[]>;
  readonly visibility: Uint8Array | null;

  constructor(
    state: RawState,
    readonly width: number,
    readonly height: number
  ) {
    this.tick = state.gameTicks;
    this.globals = state.globalVariables ?? {};
    this.visibility = state.visibility ?? null;
    for (const o of state.objects) {
      const [x, y] = o.location;
      if (x < 0 || y < 0 || o.name === "map_size") continue;
      const obj: Obj = { id: o.id, name: o.name, pos: [x, y], player: o.playerId, vars: o.variables ?? {} };
      this.objects.push(obj);
      this.byId.set(obj.id, obj);
      // the highest object on a cell wins (units on bridges)
      const k = this.key(obj.pos);
      if (!this.at.has(k) || obj.name !== "bridge") this.at.set(k, obj);
    }
    for (const o of this.objects) {
      const list = this.byName.get(o.name);
      if (list) list.push(o);
      else this.byName.set(o.name, [o]);
    }
  }

  key(c: Cell): number {
    return c[1] * this.width + c[0];
  }

  get(c: Cell): Obj | undefined {
    return this.inside(c) ? this.at.get(this.key(c)) : undefined;
  }

  of(name: string, player?: number): Obj[] {
    return (this.byName.get(name) ?? []).filter((o) => player === undefined || o.player === player);
  }

  global(name: string, player = 0): number {
    return Number(this.globals[name]?.[player] ?? 0);
  }

  /** Does the viewing player see cell c right now? */
  visible(c: Cell): boolean {
    return !this.visibility || (this.inside(c) && this.visibility[this.key(c)] === VISIBLE);
  }

  unseen(c: Cell): boolean {
    return !!this.visibility && this.inside(c) && this.visibility[this.key(c)] === UNSEEN;
  }

  unseenCount(): number {
    if (!this.visibility) return 0;
    let n = 0;
    for (const v of this.visibility) if (v === UNSEEN) n++;
    return n;
  }

  resources(player: number): number {
    return this.global("player_resources", player);
  }

  tech(variable: string, player: number): number {
    return this.global(variable, player);
  }

  camera(player: number): Cell {
    return [this.global("cam_x", player), this.global("cam_y", player)];
  }

  inside(c: Cell): boolean {
    return c[0] >= 0 && c[1] >= 0 && c[0] < this.width && c[1] < this.height;
  }

  blocked(c: Cell): boolean {
    const o = this.get(c);
    return o !== undefined && o.name !== "bridge";
  }
}

export type BfsResult = { dir: Dir | null; length: number; goal: Cell };

/** Per-tick occupancy bookkeeping so a player's own units never plan into each other. */
export class Planner {
  readonly blockedSet = new Set<number>();
  readonly reserved = new Set<number>();

  constructor(readonly world: WorldView) {
    for (const [k, o] of world.at) if (o.name !== "bridge") this.blockedSet.add(k);
  }

  free(c: Cell): boolean {
    return this.world.inside(c) && !this.blockedSet.has(this.world.key(c));
  }

  isReserved(c: Cell): boolean {
    return this.reserved.has(this.world.key(c));
  }

  reserve(c: Cell): void {
    this.reserved.add(this.world.key(c));
  }

  move(src: Cell, dst: Cell): void {
    this.blockedSet.delete(this.world.key(src));
    this.blockedSet.add(this.world.key(dst));
  }

  spawn(c: Cell): void {
    this.blockedSet.add(this.world.key(c));
  }

  /** First step of a shortest path from start to any goal, or null. */
  bfs(start: Cell, goals: Iterable<Cell>, ignoreUnits = false, limit = 4000): BfsResult | null {
    const w = this.world;
    const goalSet = new Set<number>();
    for (const g of goals) if (w.inside(g)) goalSet.add(w.key(g));
    if (goalSet.size === 0) return null;
    const sk = w.key(start);
    if (goalSet.has(sk)) return { dir: null, length: 0, goal: start };
    const passable = (k: number, c: Cell) => {
      if (!w.inside(c)) return false;
      if (ignoreUnits) {
        const o = w.at.get(k);
        return o === undefined || !STATIC.has(o.name);
      }
      return !this.blockedSet.has(k);
    };
    const prev = new Map<number, number>([[sk, -1]]);
    const queue: number[] = [sk];
    let head = 0;
    while (head < queue.length && prev.size < limit) {
      const cur = queue[head++]!;
      if (goalSet.has(cur)) {
        let k = cur;
        let length = 1;
        while (prev.get(k) !== sk) {
          k = prev.get(k)!;
          length++;
        }
        const first: Cell = [k % w.width, Math.floor(k / w.width)];
        return { dir: directionTo(start, first), length, goal: [cur % w.width, Math.floor(cur / w.width)] };
      }
      const cc: Cell = [cur % w.width, Math.floor(cur / w.width)];
      for (const n of neighbours(cc)) {
        if (!w.inside(n)) continue;
        const nk = w.key(n);
        if (prev.has(nk)) continue;
        if (passable(nk, n) || (goalSet.has(nk) && !this.blockedSet.has(nk))) {
          prev.set(nk, cur);
          queue.push(nk);
        }
      }
    }
    return null;
  }
}

/** One external action: [actionName, x, y, actionId]. */
export type PlannedAction = [string, number, number, number];
