// Replays the saved per-level solutions of a remastered game in the WASM engine:
//   node studio/griddly-demo/tools/remaster/replay_solutions.mjs <game_id> [solutions.json]
//
// The generators (sokoban.py, sokoban2.py, clusters.py, bait.py) write their per-level move lists
// (1 left, 2 up, 3 right, 4 down = the `move` action ids) to apps/web/src/games/solutions/<game_id>.json.
// Two formats are accepted (the demo's "Watch solution" button replays the same files):
//   1. puzzle solvers (sokoban, sokoban2, clusters, bait, cookmepasta, zenpuzzle):
//      [[move, ...], ...] per level - `move` action ids, seed 1
//   2. recorded bot runs (bot_explore.mjs / bot_action.mjs --record):
//      {format: 2, levels: [{seed, actions: [[actionTypeIdx, actionId], ...]}, ...]} - games with
//      random enemies replay exactly because the engine is deterministic for a given seed.
// For every level this loads it, steps the actions and asserts the GDY's own Win termination fires on
// the last action (and not before). Exits non-zero on any failure.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const id = process.argv[2];
const solPath = process.argv[3] ?? path.join(here, "../../apps/web/src/games/solutions", `${id}.json`);
const raw = JSON.parse(fs.readFileSync(solPath, "utf8"));
// normalise both formats to [{seed, actions: [[actionTypeIdx, actionId], ...]}]
const sols = Array.isArray(raw) ? raw.map((moves) => ({ seed: 1, actions: moves.map((m) => ["move", m]) })) : raw.levels;
// `spiders` (Spiders Garden) predates the remasters and lives in Mini-Grid/
const gdyFile = id === "spiders" ? "resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml"
  : `resources/games/Single-Player/Remastered/${id}.yaml`;
const gdy = fs.readFileSync(path.join(root, gdyFile), "utf8");

const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
// [name|idx, id] -> what step() takes (a bare [id] for single-action games)
const act = ([t, a]) => {
  const idx = typeof t === "string" ? actions.indexOf(t) : t;
  return actions.length > 1 ? [idx, a] : [a];
};
let failed = false;
if (sols.length !== g.getLevelCount()) {
  console.log(`${id}: ${sols.length} solutions for ${g.getLevelCount()} levels`);
  failed = true;
}
for (let l = 0; l < sols.length; l++) {
  g.loadLevel(l);
  g.seed(sols[l].seed);
  g.reset();
  const moves = sols[l].actions;
  let result = "no termination", at = -1, reward = 0;
  for (let t = 0; t < moves.length; t++) {
    const r = g.step(act(moves[t]));
    if (r && typeof r === "object") reward += Number(r.reward ?? r.rewards?.[0] ?? 0) || 0;
    if (r && r.terminated) {
      result = r.info?.playerResults?.["1"] ?? "terminated";
      at = t;
      break;
    }
  }
  const ok = result === "Win" && at === moves.length - 1;
  if (!ok) failed = true;
  console.log(`  ${id} L${l}: ${moves.length} moves -> ${result}${at >= 0 ? ` at move ${at + 1}` : ""} ${ok ? "OK" : "FAIL"}`);
}
process.exit(failed ? 1 : 0);
