// Engine state dumper for the office meme games (mplace, bufoparty, courier) - used by their
// generators' --diff checks: replays action lists and writes the state after every step.
//   node office_dump.mjs <gdy.yaml> <jobs.json> <out.json>
//   jobs = [{level, seed?, actions: [[actionName, id], ...], keep: [objectName, ...]}]
//   out  = per job: [{objs: [[name, x, y, {vars}], ...], globals: {name: value}, term}, ...] (state 0 = after reset)
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
const out = [];
for (const job of jobs) {
  const keep = new Set(job.keep);
  g.loadLevel(job.level);
  g.seed(job.seed ?? 1);
  g.reset();
  const snap = (r) => {
    const s = g.getState();
    const objs = s.objects.filter((o) => keep.has(o.name)).map((o) => [o.name, o.location[0], o.location[1], o.variables]);
    const globals = {};
    for (const [k, v] of Object.entries(s.globalVariables)) globals[k] = Array.isArray(v) ? v[0] : v;
    return { objs, globals, term: r?.terminated ? r.info?.playerResults?.["1"] ?? "end" : null };
  };
  const states = [snap(null)];
  for (const [a, id] of job.actions) {
    const r = g.step(names.length > 1 ? [names.indexOf(a), id] : [id]);
    states.push(snap(r));
    if (r?.terminated) break;
  }
  out.push(states);
}
fs.writeFileSync(process.argv[4], JSON.stringify(out));
