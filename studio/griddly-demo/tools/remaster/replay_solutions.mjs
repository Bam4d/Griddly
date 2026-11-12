// Replays solver solutions of the push-puzzle remasters in the WASM engine:
//   node studio/griddly-demo/tools/remaster/replay_solutions.mjs <game_id> [solutions.json]
//
// The generators (sokoban.py, sokoban2.py, clusters.py, bait.py) write their per-level move lists
// (1 left, 2 up, 3 right, 4 down = the `move` action ids) to tools/remaster/solutions/<game_id>.json.
// For every level this loads it, steps the moves and asserts the GDY's own Win termination fires on
// the last move (and not before). Exits non-zero on any failure.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const id = process.argv[2];
const solPath = process.argv[3] ?? new URL(`./solutions/${id}.json`, import.meta.url).pathname;
const sols = JSON.parse(fs.readFileSync(solPath, "utf8"));
const gdy = fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered", `${id}.yaml`), "utf8");

const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const act = (a) => (actions.length > 1 ? [actions.indexOf("move"), a] : [a]);
let failed = false;
if (sols.length !== g.getLevelCount()) {
  console.log(`${id}: ${sols.length} solutions for ${g.getLevelCount()} levels`);
  failed = true;
}
for (let l = 0; l < sols.length; l++) {
  g.loadLevel(l);
  g.seed(1);
  g.reset();
  const moves = sols[l];
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
