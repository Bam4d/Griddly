// Helper for lumberjack.py --diff: replays action lists in the WASM engine and writes the state after
// every step as JSON, so the Python simulator (lumberjack_sim.py) can be checked against the GDY.
//   node lumberjack_dump.mjs <gdy.yaml> <jobs.json> <out.json>   jobs = [{level, actions: [[name, id], ...]}]
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
const names = g.getActionNames();
const KEEP = new Set(["tree", "oak", "oakcut", "water", "deep", "bramble", "bush", "logpile", "hen", "bridge", "stump",
  "fire", "bridgefire", "campfire", "avatar"]);
const snap = () => {
  const s = g.getState();
  const objs = {};
  let logs = null, bucket = null;
  for (const o of s.objects) {
    if (!KEEP.has(o.name)) continue;
    (objs[o.name] ??= []).push(o.location);
    if (o.name === "avatar") { logs = o.variables.logs; bucket = o.variables.bucket; }
  }
  for (const k of Object.keys(objs)) objs[k].sort((a, b) => a[0] - b[0] || a[1] - b[1]);
  return { objs, logs, bucket };
};
const out = [];
for (const job of jobs) {
  g.loadLevel(job.level);
  g.seed(1);
  g.reset();
  const states = [snap()];
  for (const [a, id] of job.actions) {
    const r = g.step([names.indexOf(a), id]);
    states.push(snap());
    if (r?.terminated) break;
  }
  out.push(states);
}
fs.writeFileSync(process.argv[4], JSON.stringify(out));
