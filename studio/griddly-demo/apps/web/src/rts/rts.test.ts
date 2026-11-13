import { describe, expect, test } from "bun:test";

import { RTS_ACTIONS, RTS_LEVELS, chordToActionId, commandKey, keyToActionId } from "./gdy";
import { OrderBook, formation } from "./orders";
import { cellAt, viewport } from "./view";
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
const globals = { player_resources: { "0": 0, "1": 20, "2": 9 }, cam_x: { "1": 1 }, cam_y: { "1": 1 }, res_mining: { "1": 2 } };
const world = () => new WorldView(state, globals, 8, 6);

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
    // the second one is not affordable in the same tick (20 - 12 < 12): it stays queued
    expect(book.queues.get(1)).toEqual([{ kind: "train", unit: "tank" }]);
  });

  test("formation spreads a group over free cells", () => {
    const cells = formation(world(), [0, 4], 3);
    expect(new Set(cells.map((c) => c.join())).size).toBe(3);
  });
});

describe("rts view", () => {
  test("viewport clamps to the map and maps clicks to cells", () => {
    const v = viewport([60, 30], 64, 32, 640, 320, 32);
    expect(v.cols).toBe(20);
    expect(v.x0).toBe(44);
    expect(v.y0).toBe(22);
    expect(cellAt(v, 33, 1)).toEqual([45, 22]);
  });
});
