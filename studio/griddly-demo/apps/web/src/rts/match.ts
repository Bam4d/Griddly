/**
 * One RTS match on the Griddly engine: the human player's order book (or an AI, to spectate) as
 * player 1 and the scripted AI as player 2.
 *
 * Time belongs to the engine: its game clock (GDY Environment.TickRate) runs the ticks. Between
 * ticks each player records its actions with `queueActions`; on every tick the engine runs all of
 * them together and advances time once. This class only decides what to queue: `plan()` asks both
 * players (each from its own fog-filtered player state) for their next actions, and the page calls
 * `advance(seconds)` every frame so the engine's clock can run whatever ticks are due.
 * Camera pans are GDY actions too (move_camera), applied at once without advancing time.
 */
import type { Griddly } from "@griddly/wasm";

import { RTSPolicy, STRATEGIES } from "./ai";
import type { Strategy } from "./ai";
import { OrderBook } from "./orders";
import type { TickPlan } from "./orders";
import { DIRS, DOWN, LEFT, RIGHT, UNITS, UP, WorldView, manhattan } from "./world";
import type { Cell, Dir, PlannedAction, RawState } from "./world";

/** The subset of the engine API a match needs (lets tests swap in a fake). */
export type Engine = Pick<
  Griddly,
  | "loadLevel"
  | "reset"
  | "seed"
  | "getPlayerState"
  | "getActionNames"
  | "stepPlayer"
  | "queueActions"
  | "clearQueuedActions"
  | "advanceTime"
  | "tick"
  | "setTickRate"
  | "getTickRate"
  | "setPaused"
  | "getAvailableActionNames"
  | "getAvailableActionIds"
> & { readonly width: number; readonly height: number };

export type MatchOptions = {
  level: number;
  seed?: number;
  /** The AI opponent's strategy (player 2). */
  enemy?: Strategy;
  /** Let an AI play player 1 too (spectator mode). */
  autopilot?: Strategy | null;
  attackSize?: number;
};

export type Outcome = "playing" | "won" | "lost" | "draw";

export class Match {
  readonly orders = new OrderBook(1);
  readonly ai: RTSPolicy;
  autopilot: RTSPolicy | null;
  readonly actionNames: string[];
  /** What player 1 (you) knows: the engine's fog-filtered player state. */
  world: WorldView;
  /** What player 2 (the AI) knows; the AI never sees anything else. */
  enemyWorld: WorldView;
  outcome: Outcome = "playing";
  lastAi: { actions: PlannedAction[]; caption: string[] } = { actions: [], caption: [] };
  lastHuman: PlannedAction[] = [];
  unitStatus = new Map<number, string>();
  /** Rally points of player 1's production buildings: new units get a "smart" order to go there. */
  readonly rallies = new Map<number, Cell>();
  /** Player 1 objects that appeared in the last tick. */
  spawned: number[] = [];
  private known = new Set<number>();
  /** Names of the actions each of player 1's objects can do right now, keyed "x,y" (from the engine). */
  private available: Record<string, string[]> | null = null;

  constructor(
    readonly engine: Engine,
    readonly options: MatchOptions
  ) {
    engine.loadLevel(options.level);
    engine.seed(options.seed ?? 1);
    engine.reset();
    this.actionNames = engine.getActionNames();
    this.ai = new RTSPolicy(2, 1, { strategy: options.enemy ?? STRATEGIES.ARMOUR, attackSize: options.attackSize, seed: options.seed });
    this.autopilot = options.autopilot ? new RTSPolicy(1, 2, { strategy: options.autopilot, seed: options.seed }) : null;
    this.world = this.read();
    this.enemyWorld = this.view(2);
    // a new order is recorded for the very next tick
    this.orders.onChange = () => this.replanHuman();
    this.known = new Set(this.world.objects.filter((o) => o.player === 1).map((o) => o.id));
  }

  /** A player's view of the game: only what that player can know (fog of war in the engine). */
  view(playerId: number): WorldView {
    return new WorldView(this.engine.getPlayerState(playerId) as unknown as RawState, this.engine.width, this.engine.height);
  }

  /** Re-read player 1's state. */
  read(): WorldView {
    this.world = this.view(1);
    this.available = null;
    return this.world;
  }

  private toEngine(actions: PlannedAction[]): number[][] {
    const out: number[][] = [];
    for (const [name, x, y, id] of actions) {
      const idx = this.actionNames.indexOf(name);
      if (idx >= 0) out.push([x, y, idx, id]);
    }
    return out;
  }

  setAutopilot(strategy: Strategy | null): void {
    this.autopilot = strategy ? new RTSPolicy(1, 2, { strategy, seed: this.options.seed }) : null;
  }

  /**
   * Record both players' actions for the next engine tick (replacing anything recorded before).
   * Each player decides from its own state: you from player 1's, the AI from player 2's.
   */
  plan(): void {
    if (this.outcome !== "playing") return;
    const world = this.world;
    let human: PlannedAction[];
    if (this.autopilot) {
      human = this.autopilot.act(world).actions.filter((a) => a[0] !== "move_camera");
      this.unitStatus = new Map();
    } else {
      const plan = this.orders.plan(world);
      human = plan.actions;
      this.unitStatus = plan.status;
      this.pendingOrders = plan;
    }
    // the AI decides from its own player state only
    this.enemyWorld = this.view(2);
    this.lastAi = this.ai.act(this.enemyWorld);
    this.lastHuman = human;
    this.planned = true;
    this.engine.clearQueuedActions(-1);
    this.engine.queueActions(0, this.toEngine(human));
    this.engine.queueActions(1, this.toEngine(this.lastAi.actions));
  }

  /**
   * Let `seconds` of real time pass on the engine's clock. When a tick ran, read the new state and
   * record both players' next actions. Returns the number of ticks that ran.
   */
  advance(seconds: number): number {
    if (this.outcome !== "playing") return 0;
    const result = this.engine.advanceTime(seconds);
    if (result.ticks > 0) this.afterTick(result);
    return result.ticks;
  }

  /** Run one tick now (single-stepping, headless runs): plans first if nothing was recorded. */
  tick(): Outcome {
    if (this.outcome !== "playing") return this.outcome;
    if (!this.planned) this.plan();
    this.afterTick(this.engine.tick());
    return this.outcome;
  }

  private planned = false;
  /** Player 1's order queues once the recorded actions have run (committed after the tick). */
  private pendingOrders: TickPlan | null = null;

  private afterTick(result: { terminated: boolean; info?: { playerResults?: Record<string, string> } }): void {
    if (this.pendingOrders) this.orders.commit(this.pendingOrders);
    this.pendingOrders = null;
    this.read();
    this.sendToRally();
    if (result.terminated) {
      const r = result.info?.playerResults?.["1"];
      this.outcome = r === "Win" ? "won" : r === "Lose" ? "lost" : "draw";
      return;
    }
    // record the next tick's actions straight away; orders given before the tick replace them
    this.plan();
  }

  /**
   * Re-record player 1's actions after a new order, so the engine runs it on the next tick. The
   * order book is planned on a copy so a unit that finishes an order early is not advanced twice.
   */
  private replanHuman(): void {
    if (this.outcome !== "playing" || this.autopilot) return;
    const plan = this.orders.plan(this.world);
    this.unitStatus = plan.status;
    this.lastHuman = plan.actions;
    this.pendingOrders = plan;
    this.engine.clearQueuedActions(0);
    this.engine.queueActions(0, this.toEngine(plan.actions));
  }

  setTickRate(ticksPerSecond: number): void {
    this.engine.setTickRate(ticksPerSecond);
  }

  get tickRate(): number {
    return this.engine.getTickRate();
  }

  setPaused(paused: boolean): void {
    this.engine.setPaused(paused);
  }

  /** New units walk to their building's rally point (a resource cell makes workers harvest it). */
  private sendToRally(): void {
    const mine = this.world.objects.filter((o) => o.player === 1);
    this.spawned = mine.filter((o) => !this.known.has(o.id)).map((o) => o.id);
    this.known = new Set(mine.map((o) => o.id));
    for (const id of this.spawned) {
      const u = this.world.byId.get(id);
      if (!u || !UNITS.includes(u.name)) continue;
      for (const [bid, rally] of this.rallies) {
        const b = this.world.byId.get(bid);
        if (!b) {
          this.rallies.delete(bid);
          continue;
        }
        if (manhattan(b.pos, u.pos) !== 1) continue;
        const order = this.orders.smartOrder(this.world, u, rally);
        if (order) this.orders.set(u.id, order);
        break;
      }
    }
  }

  /** Pan player 1's GDY camera (cam_x / cam_y) toward `target`, without advancing time. */
  panCameraTo(target: Cell): void {
    const base = this.world.of("base", 1)[0];
    if (!base) return;
    let cam = this.world.camera(1);
    const pans: PlannedAction[] = [];
    for (let i = 0; i < 64; i++) {
      const dx = target[0] - cam[0];
      const dy = target[1] - cam[1];
      if (Math.abs(dx) <= 1 && Math.abs(dy) <= 1) break;
      const d: Dir = Math.abs(dx) >= Math.abs(dy) ? (dx > 0 ? RIGHT : LEFT) : dy > 0 ? DOWN : UP;
      pans.push(["move_camera", base.pos[0], base.pos[1], d]);
      cam = [cam[0] + 2 * DIRS[d][0], cam[1] + 2 * DIRS[d][1]];
    }
    this.panCamera(pans.map((p) => p[3] as Dir));
  }

  /** Issue move_camera actions (2 cells each) for player 1 without advancing time. */
  panCamera(dirs: Dir[]): void {
    const base = this.world.of("base", 1)[0];
    if (!base || dirs.length === 0) return;
    this.engine.stepPlayer(0, this.toEngine(dirs.map((d) => ["move_camera", base.pos[0], base.pos[1], d])), false);
    this.read();
  }

  /** What the engine says player 1's object at `cell` can do right now: {action: [valid ids]}. */
  availableAt(cell: Cell): Record<string, number[]> {
    this.available ??= this.engine.getAvailableActionNames(1);
    const names = (this.available[`${cell[0]},${cell[1]}`] ?? []).filter((n) => n !== "move_camera");
    if (names.length === 0) return {};
    const ids = this.engine.getAvailableActionIds(cell[0], cell[1], names);
    const out: Record<string, number[]> = {};
    for (const n of names) out[n] = ids[n] ?? [];
    return out;
  }
}
