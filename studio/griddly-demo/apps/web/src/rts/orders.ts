/**
 * The human player's order queues. Mouse clicks become orders ("move here", "attack that",
 * "harvest there", "build a tower here"); every tick each unit's current order is turned into at
 * most one external GDY action. The engine validates and applies it, exactly as for the AI.
 */
import { COSTS, DIRS, DIR_LIST, FIGHTERS, Planner, STATIC, TARGET_PRIORITY, UNITS, directionTo, manhattan, minBy, neighbours, sameCell, step } from "./world";
import type { Cell, Dir, Obj, PlannedAction, WorldView } from "./world";

export type Order =
  | { kind: "move"; to: Cell; stuck?: number }
  | { kind: "attack"; target: number; last: Cell }
  | { kind: "harvest"; at: Cell; resource: string }
  | { kind: "build"; building: string; site: Cell }
  | { kind: "assist"; action: "heal" | "repair"; target: number }
  | { kind: "capture"; relay: Cell }
  | { kind: "train"; unit: string }
  | { kind: "research"; tech: number }
  | { kind: "direct"; action: string; id: number };

export const RESOURCES = ["minerals", "gems", "forest"];
const PRODUCTION = ["base", "barracks"];

export const describeOrder = (o: Order): string => {
  switch (o.kind) {
    case "move":
      return `move to ${o.to[0]},${o.to[1]}`;
    case "attack":
      return "attack";
    case "harvest":
      return o.resource === "forest" ? "chop wood" : `harvest ${o.resource}`;
    case "build":
      return `build ${o.building}`;
    case "assist":
      return o.action;
    case "capture":
      return "capture relay";
    case "train":
      return `train ${o.unit}`;
    case "research":
      return "research";
    case "direct":
      return o.action.replace("_", " ");
  }
};

/**
 * One tick's plan: the actions to record, each unit's status, and the order queues as they will be
 * once those actions have run (finished orders removed). Commit `queues` only after the tick ran.
 */
export type TickPlan = { actions: PlannedAction[]; status: Map<number, string>; queues: Map<number, Order[]> };

/**
 * `n` distinct walkable cells closest to `cell` (breadth-first through non-static cells), so a
 * group ordered to one spot spreads out around it instead of queueing for the same cell.
 */
export function formation(world: WorldView, cell: Cell, n: number, movers: Set<number> = new Set()): Cell[] {
  const open = (c: Cell) => {
    const o = world.get(c);
    return o === undefined || o.name === "bridge" || movers.has(o.id);
  };
  const passable = (c: Cell) => {
    const o = world.get(c);
    return o === undefined || !STATIC.has(o.name) || o.name === "bridge";
  };
  const out: Cell[] = [];
  const seen = new Set<number>([world.key(cell)]);
  const queue: Cell[] = [cell];
  for (let head = 0; head < queue.length && out.length < n && seen.size < 2000; head++) {
    const c = queue[head]!;
    if (open(c)) out.push(c);
    for (const nb of neighbours(c)) {
      if (!world.inside(nb) || seen.has(world.key(nb)) || !passable(nb)) continue;
      seen.add(world.key(nb));
      queue.push(nb);
    }
  }
  while (out.length < n) out.push(cell);
  return out;
}

/** Turns order queues into this tick's actions for one player. */
export class OrderBook {
  queues = new Map<number, Order[]>();
  autoFight = true;
  /** Called when orders change, so the next tick's actions can be recorded again. */
  onChange: (() => void) | null = null;

  constructor(readonly pid: number) {}

  set(unit: number, order: Order, append = false): void {
    const q = append ? (this.queues.get(unit) ?? []) : [];
    q.push(order);
    this.queues.set(unit, q);
    this.onChange?.();
  }

  clear(unit: number): void {
    this.queues.delete(unit);
    this.onChange?.();
  }

  current(unit: number): Order | undefined {
    return this.queues.get(unit)?.[0];
  }

  /** The order to give a unit for a right click on `cell` ("smart" order), or null. */
  smartOrder(world: WorldView, unit: Obj, cell: Cell): Order | null {
    const o = world.get(cell);
    if (o && o.player !== 0 && o.player !== this.pid && o.name in TARGET_PRIORITY && unit.name !== "medic") {
      return { kind: "attack", target: o.id, last: o.pos };
    }
    if (o && unit.name === "worker" && RESOURCES.includes(o.name)) return { kind: "harvest", at: cell, resource: o.name };
    if (o && unit.name === "worker" && o.player === this.pid && o.name === "base") return { kind: "harvest", at: cell, resource: "deposit" };
    if (o && unit.name === "worker" && o.player === this.pid && (o.vars.health ?? 0) < (o.vars.max_health ?? 0) && !UNITS.includes(o.name)) {
      return { kind: "assist", action: "repair", target: o.id };
    }
    if (o && unit.name === "medic" && o.player === this.pid && UNITS.includes(o.name)) return { kind: "assist", action: "heal", target: o.id };
    if (o && (o.name === "relay" || (o.name === "relay_owned" && o.player !== this.pid))) return { kind: "capture", relay: cell };
    return { kind: "move", to: cell };
  }

  /**
   * Plan this tick's actions for every unit with an order (plus idle self-defence). Works on a copy
   * of the order queues, so planning again before the tick (after a new order) loses nothing.
   */
  plan(world: WorldView): TickPlan {
    const real = this.queues;
    this.queues = structuredClone(real);
    try {
      return { ...this.planOn(world), queues: this.queues };
    } finally {
      this.queues = real;
    }
  }

  /** The tick ran: the planned queues become the real ones (orders given since were re-planned). */
  commit(plan: TickPlan): void {
    this.queues = plan.queues;
  }

  private planOn(world: WorldView): { actions: PlannedAction[]; status: Map<number, string> } {
    const plan = new Planner(world);
    const actions: PlannedAction[] = [];
    const status = new Map<number, string>();
    const mine = world.objects.filter((o) => o.player === this.pid);
    const alive = new Set(mine.map((o) => o.id));
    for (const id of [...this.queues.keys()]) if (!alive.has(id)) this.queues.delete(id);
    let res = world.resources(this.pid);

    // buildings first (they never move), then units
    const ordered = [...mine].sort((a, b) => Number(UNITS.includes(a.name)) - Number(UNITS.includes(b.name)));
    for (const u of ordered) {
      const unit = { ...u };
      let guard = 0;
      while (guard++ < 4) {
        const order = this.current(u.id);
        if (!order) {
          if (this.autoFight && UNITS.includes(u.name)) this.idle(world, actions, unit);
          break;
        }
        const r = this.exec(world, plan, actions, unit, order, res);
        res = r.res;
        if (r.status) status.set(u.id, r.status);
        if (r.done) {
          this.queues.get(u.id)?.shift();
          if (!this.queues.get(u.id)?.length) this.queues.delete(u.id);
          if (r.acted) break;
          continue;
        }
        break;
      }
    }
    return { actions, status };
  }

  /** Idle units hit adjacent enemies; gunners shoot enemies 2 cells away. */
  private idle(world: WorldView, actions: PlannedAction[], u: Obj) {
    if (u.name === "medic") return;
    const enemyOn = (c: Cell) => {
      const o = world.get(c);
      return o && o.player !== 0 && o.player !== this.pid && o.name in TARGET_PRIORITY ? o : undefined;
    };
    if (u.name === "marine") {
      const shots = DIR_LIST.filter((k) => enemyOn([u.pos[0] + 2 * DIRS[k][0], u.pos[1] + 2 * DIRS[k][1]]));
      if (shots.length) {
        actions.push(["shoot", u.pos[0], u.pos[1], shots[0]!]);
        return;
      }
    }
    const adj = DIR_LIST.filter((k) => enemyOn(step(u.pos, k)));
    if (adj.length && (FIGHTERS.includes(u.name) || u.name === "scout" || u.name === "worker")) {
      const best = minBy(adj, (k) => TARGET_PRIORITY[enemyOn(step(u.pos, k))!.name]!)!;
      actions.push(["attack", u.pos[0], u.pos[1], best]);
    }
  }

  private walk(world: WorldView, plan: Planner, actions: PlannedAction[], unit: Obj, goals: Cell[]): boolean {
    if (unit.name === "tank" && unit.vars.cooldown === 1) return true; // reloading its tracks: wait
    let s = plan.bfs(unit.pos, goals, false, 6000);
    if (s === null) s = plan.bfs(unit.pos, goals, true, 6000);
    if (s === null || s.dir === null) return false;
    const nxt = step(unit.pos, s.dir);
    if (!plan.free(nxt)) return true; // blocked by a unit this tick: wait
    plan.move(unit.pos, nxt);
    actions.push(["move", unit.pos[0], unit.pos[1], s.dir]);
    unit.pos = nxt;
    void world;
    return true;
  }

  private exec(
    world: WorldView,
    plan: Planner,
    actions: PlannedAction[],
    unit: Obj,
    order: Order,
    res: number
  ): { done: boolean; acted?: boolean; res: number; status?: string } {
    const pos = unit.pos;
    const act = (name: string, d: Dir | number) => actions.push([name, pos[0], pos[1], d]);
    switch (order.kind) {
      case "move": {
        if (sameCell(pos, order.to)) return { done: true, res };
        const occupant = world.get(order.to);
        if (occupant && manhattan(pos, order.to) <= 1) return { done: true, res };
        if (occupant && STATIC.has(occupant.name) && occupant.name !== "bridge") {
          // walk next to an obstacle
          if (manhattan(pos, order.to) === 1) return { done: true, res };
          return this.walk(world, plan, actions, unit, neighbours(order.to)) ? { done: false, res, status: "moving" } : { done: true, res };
        }
        const before = unit.pos;
        if (!this.walk(world, plan, actions, unit, [order.to])) return { done: true, res, status: "no path" };
        if (sameCell(before, unit.pos)) {
          order.stuck = (order.stuck ?? 0) + 1;
          if (order.stuck > 8 && manhattan(pos, order.to) <= 3) return { done: true, res };
        } else order.stuck = 0;
        return { done: false, res, status: "moving" };
      }
      case "attack": {
        const t = world.byId.get(order.target);
        if (!t) return { done: true, res };
        order.last = t.pos;
        if (unit.name === "marine") {
          for (const k of DIR_LIST) {
            if (sameCell([pos[0] + 2 * DIRS[k][0], pos[1] + 2 * DIRS[k][1]], t.pos)) {
              act("shoot", k);
              return { done: false, acted: true, res, status: "shooting" };
            }
          }
        }
        const d = directionTo(pos, t.pos);
        if (d !== null) {
          act("attack", d);
          return { done: false, acted: true, res, status: "attacking" };
        }
        const goals = neighbours(t.pos);
        if (unit.name === "marine") goals.push(...DIR_LIST.map((k) => [t.pos[0] + 2 * DIRS[k][0], t.pos[1] + 2 * DIRS[k][1]] as Cell));
        return this.walk(world, plan, actions, unit, goals.filter((g) => world.inside(g))) ? { done: false, res, status: "closing in" } : { done: true, res };
      }
      case "harvest": {
        if (unit.name !== "worker") return { done: true, res };
        const base = minBy(world.of("base", this.pid), (b) => manhattan(b.pos, pos));
        if (unit.vars.carrying) {
          if (!base) return { done: true, res };
          const d = directionTo(pos, base.pos);
          if (d !== null) {
            act("gather", d);
            if (order.resource === "deposit") return { done: true, acted: true, res };
            return { done: false, acted: true, res, status: "depositing" };
          }
          this.walk(world, plan, actions, unit, neighbours(base.pos));
          return { done: false, res, status: "returning cargo" };
        }
        if (order.resource === "deposit") return { done: true, res };
        let target = world.get(order.at);
        if (!target || target.name !== order.resource) {
          // depleted: move on to the nearest deposit of the same kind close by
          const next = minBy(
            world.of(order.resource).filter((r) => manhattan(r.pos, order.at) <= 8),
            (r) => manhattan(r.pos, order.at)
          );
          if (!next) return { done: true, res, status: "depleted" };
          order.at = next.pos;
          target = next;
        }
        const d = directionTo(pos, target.pos);
        if (d !== null) {
          plan.reserve(pos);
          act("gather", d);
          return { done: false, acted: true, res, status: order.resource === "forest" ? "chopping" : "mining" };
        }
        const slots = neighbours(target.pos).filter((n) => plan.free(n) && !plan.isReserved(n));
        if (slots.length === 0) {
          // every slot is taken: try another deposit of the same kind
          const alt = minBy(
            world.of(order.resource).filter((r) => manhattan(r.pos, order.at) <= 8 && neighbours(r.pos).some((n) => plan.free(n) && !plan.isReserved(n))),
            (r) => manhattan(r.pos, pos)
          );
          if (alt) order.at = alt.pos;
          return { done: false, res, status: "waiting for a free slot" };
        }
        const s = plan.bfs(pos, slots, false, 6000);
        if (s) plan.reserve(s.goal);
        this.walk(world, plan, actions, unit, s ? [s.goal] : slots);
        return { done: false, res, status: "to the deposit" };
      }
      case "build": {
        const occupant = world.get(order.site);
        if (occupant && !UNITS.includes(occupant.name)) return { done: true, res };
        plan.reserve(order.site);
        const d = directionTo(pos, order.site);
        if (d === null) {
          const goals = neighbours(order.site).filter((n) => world.inside(n));
          return this.walk(world, plan, actions, unit, goals) ? { done: false, res, status: `to the ${order.building} site` } : { done: true, res, status: "no path" };
        }
        if (occupant) return { done: false, res, status: "site is blocked" };
        const cost = COSTS[order.building] ?? 0;
        if (res < cost) return { done: false, res, status: `needs ${cost} resources` };
        act(`build_${order.building}`, d);
        plan.spawn(order.site);
        return { done: true, acted: true, res: res - cost };
      }
      case "assist": {
        const t = world.byId.get(order.target);
        if (!t || (t.vars.health ?? 0) >= (t.vars.max_health ?? 0)) return { done: true, res };
        const d = directionTo(pos, t.pos);
        if (d !== null) {
          act(order.action, d);
          return { done: false, acted: true, res, status: order.action === "heal" ? "healing" : "repairing" };
        }
        return this.walk(world, plan, actions, unit, neighbours(t.pos)) ? { done: false, res, status: "on the way" } : { done: true, res };
      }
      case "capture": {
        const r = world.get(order.relay);
        if (!r || (r.name === "relay_owned" && r.player === this.pid)) return { done: true, res };
        if (manhattan(pos, order.relay) === 1) {
          plan.reserve(pos);
          return { done: false, res, status: "capturing" };
        }
        return this.walk(world, plan, actions, unit, neighbours(order.relay)) ? { done: false, res, status: "to the relay" } : { done: true, res };
      }
      case "train": {
        if (!PRODUCTION.includes(unit.name)) return { done: true, res };
        const cost = COSTS[order.unit] ?? 0;
        if (res < cost) return { done: false, res, status: `needs ${cost} resources` };
        const d = DIR_LIST.find((k) => plan.free(step(pos, k)));
        if (d === undefined) return { done: false, res, status: "no room to deploy" };
        act(`build_${order.unit}`, d);
        plan.spawn(step(pos, d));
        return { done: true, acted: true, res: res - cost };
      }
      case "research": {
        act("research", order.tech);
        return { done: true, acted: true, res };
      }
      case "direct": {
        act(order.action, order.id);
        return { done: true, acted: true, res };
      }
    }
  }
}
