import { describe, expect, test } from "bun:test";

import { RTS_ACTIONS, RTS_LEVELS, chordToActionId, commandKey, keyToActionId } from "./gdy";
import { OrderBook, formation } from "./orders";
import { cellAt, isVisible, viewport } from "./view";
import { Planner, WorldView } from "./world";
import type { RawState } from "./world";

// A tiny hand-made state: player 1 base + worker, a mineral, a rock and an enemy trooper.
const state: RawState = {
  gameTicks: 3,
  objects: [
    { id: 1, name: "base", location: [1, 1], playerId: 1, variables: { health: 50, max_health: 50 } },
    { id: 2, name: "worker", location: [2, 1], playerId: 1, variables: { health: 6, max_health: 6, carrying: 0 } },
    { id: 3, name: "minerals", location: [5, 1], playerId: 0, variables: { resources: 15 } },
    { id: 4, name: "rock", location: [3, 0], playerId: 0 },
    { id: 5, name: "rock", location: [3, 1], playerId: 0 },
    { id: 6, name: "combat", location: [6, 4], playerId: 2, variables: { health: 12, max_health: 12 } },
    { id: 7, name: "map_size", location: [7, 5], playerId: 0 },
  ],
};
// a player 1 state from the engine: per-player globals are indexed by player id
const globals = { player_resources: [0, 20, 0], cam_x: [0, 1, 0], cam_y: [0, 1, 0], res_mining: [0, 2, 0] };
const world = () => new WorldView({ ...state, globalVariables: globals, visibility: null }, 8, 6);

describe("rts gdy metadata", () => {
  test("the GDY ships 7 large maps", () => {
    expect(RTS_LEVELS.map((l) => l.name)).toEqual(["Crossroads", "River Delta", "Highlands", "Archipelago", "Deep Forest", "Desert Canyon", "Twin Fortresses"]);
    for (const l of RTS_LEVELS) expect(l.width * l.height).toBeGreaterThanOrEqual(1500);
  });

  test("suggested key bindings map to action ids", () => {
    expect(keyToActionId(RTS_ACTIONS.move_camera, "ArrowLeft")).toBe(1);
    expect(keyToActionId(RTS_ACTIONS.move_camera, "ArrowDown")).toBe(4);
    expect(commandKey(RTS_ACTIONS.build_barracks)).toBe("b");
    expect(chordToActionId(RTS_ACTIONS.build_barracks, "b", "d")).toBe(3);
    expect(chordToActionId(RTS_ACTIONS.research, "r", "5")).toBe(5);
    expect(RTS_ACTIONS.dash?.internal).toBe(true);
  });
});

describe("rts world", () => {
  test("reads objects, globals and skips the map-size marker", () => {
    const w = world();
    expect(w.resources(1)).toBe(20);
    expect(w.tech("res_mining", 1)).toBe(2);
    expect(w.camera(1)).toEqual([1, 1]);
    expect(w.of("map_size")).toEqual([]);
    expect(w.get([2, 1])?.name).toBe("worker");
  });

  test("planner BFS routes around rocks", () => {
    const w = world();
    const step = new Planner(w).bfs([2, 1], [[4, 1]]);
    expect(step?.dir).toBe(4); // down: (3,0) and (3,1) are rock
    expect(step?.length).toBe(4);
  });
});

describe("rts orders", () => {
  test("right-click semantics: harvest a mineral, attack an enemy, move elsewhere", () => {
    const w = world();
    const book = new OrderBook(1);
    const worker = w.byId.get(2)!;
    expect(book.smartOrder(w, worker, [5, 1])).toEqual({ kind: "harvest", at: [5, 1], resource: "minerals" });
    expect(book.smartOrder(w, worker, [6, 4])).toMatchObject({ kind: "attack", target: 6 });
    expect(book.smartOrder(w, worker, [0, 5])).toEqual({ kind: "move", to: [0, 5] });
  });

  test("a harvest order walks to the deposit, one action per unit per tick", () => {
    const w = world();
    const book = new OrderBook(1);
    book.set(2, { kind: "harvest", at: [5, 1], resource: "minerals" });
    const { actions } = book.plan(w);
    expect(actions).toEqual([["move", 2, 1, 4]]);
  });

  test("train orders wait until the player can afford them, then deploy to a free side", () => {
    const book = new OrderBook(1);
    book.set(1, { kind: "train", unit: "tank" }); // 12
    book.set(1, { kind: "train", unit: "tank" }, true);
    const first = book.plan(world());
    expect(first.actions).toEqual([["build_tank", 1, 1, 1]]);
    // planning does not consume orders: planning again (after a new order) gives the same actions
    expect(book.plan(world()).actions).toEqual(first.actions);
    expect(book.queues.get(1)?.length).toBe(2);
    // once the tick ran, the plan is committed: the second one was not affordable in the same tick
    // (20 - 12 < 12), so it stays queued
    book.commit(first);
    expect(book.queues.get(1)).toEqual([{ kind: "train", unit: "tank" }]);
  });

  test("formation spreads a group over free cells", () => {
    const cells = formation(world(), [0, 4], 3);
    expect(new Set(cells.map((c) => c.join())).size).toBe(3);
  });
});

describe("rts view", () => {
  test("the engine's camera window is scaled to fit and clicks map back to cells", () => {
    // engine view: 24x15 cells starting at cell (40, 17) (its camera origin), canvas 960x700
    const v = viewport([40, 17], 24, 15, 960, 700);
    expect(v.tile).toBe(40);
    expect(v.ox).toBe(0);
    expect(v.oy).toBe(50); // centred vertically
    expect(cellAt(v, 41, 50 + 1)).toEqual([41, 17]);
    expect(cellAt(v, 959, 50 + 599)).toEqual([63, 31]);
  });

  test("visibility comes from the engine's fog state", () => {
    const w = world();
    const fog = new Uint8Array(8 * 6);
    fog[w.key([2, 1])] = 2;
    fog[w.key([3, 1])] = 1;
    expect(isVisible(w, fog, [2, 1])).toBe(true);
    expect(isVisible(w, fog, [3, 1])).toBe(false); // explored only
    expect(isVisible(w, null, [3, 1])).toBe(true); // no fog
  });
});
