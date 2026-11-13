// Helper for frostflame.py --diff: replays move lists in the WASM engine and prints the state after
// every move as JSON lines, so the Python simulator (frostflame_sim.py) can be checked against the GDY.
//   node frostflame_dump.mjs <gdy.yaml> <jobs.json> <out.json>   jobs = [{level, moves: [1..4, ...]}, ...]
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));
const gdy = fs.readFileSync(process.argv[2], "utf8");
const jobs = JSON.parse(fs.readFileSync(process.argv[3], "utf8"));
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const actions = g.getActionNames();
const KEEP = new Set(["avatar", "cube", "brazier", "ice", "pond", "snowman", "frostwall", "socket", "beacon", "shrine"]);
const snap = (r) => {
  const s = g.getState();
  const o = {};
  for (const x of s.objects) if (KEEP.has(x.name)) (o[x.name] ??= []).push(x.location);
  return { objs: o, term: r?.terminated ? r.info?.playerResults?.["1"] ?? "end" : null };
};
const out = [];
for (const job of jobs) {
  g.loadLevel(job.level);
  g.seed(1);
  g.reset();
  const states = [snap(null)];
  for (const m of job.moves) {
    const r = g.step(actions.length > 1 ? [actions.indexOf("move"), m] : [m]);
    states.push(snap(r));
    if (r?.terminated) break;
  }
  out.push(states);
}
fs.writeFileSync(process.argv[4], JSON.stringify(out));
