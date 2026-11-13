// Generic engine state dumper used by the meme-games-b generators (chatonfat, rackpacker, oom) to
// cross-check their Python simulators against the WASM engine:
//   node meme_dump.mjs <gdy.yaml> <jobs.json> <out.json>
// jobs = [{level, seed?, actions: [[actionName, id], ...], keep?: [objectNames], vars?: [avatarVarNames]}]
// out  = per job: list of states (after reset, then after every step until termination):
//        {objs: {name: [[x, y], ...sorted]}, vars: {name: value}, globals: {name: value}, term: "Win"|"Lose"|null}
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));
const gdy = fs.readFileSync(process.argv[2], "utf8");
const jobs = JSON.parse(fs.readFileSync(process.argv[3], "utf8"));
const avatarName = gdy.match(/AvatarObject:\s*(\S+)/)[1];
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const names = g.getActionNames();
const snap = (job, term) => {
  const s = g.getState();
  const keep = job.keep ? new Set(job.keep) : null;
  const objs = {};
  const vars = {};
  for (const o of s.objects) {
    if (o.name.startsWith("_")) continue;
    if (o.name === avatarName) for (const v of job.vars ?? []) vars[v] = o.variables[v];
    if (keep && !keep.has(o.name)) continue;
    (objs[o.name] ??= []).push(o.location);
  }
  for (const k of Object.keys(objs)) objs[k].sort((a, b) => a[0] - b[0] || a[1] - b[1]);
  const globals = {};
  for (const [k, v] of Object.entries(s.globalVariables ?? {})) if (!k.startsWith("_")) globals[k] = v[0];
  return { objs, vars, globals, term };
};
const out = [];
for (const job of jobs) {
  g.loadLevel(job.level);
  g.seed(job.seed ?? 1);
  g.reset();
  const states = [snap(job, null)];
  for (const [a, id] of job.actions) {
    const r = g.step([names.indexOf(a), id]);
    const term = r?.terminated ? (r.info?.playerResults?.["1"] ?? "end") : null;
    states.push(snap(job, term));
    if (term) break;
  }
  out.push(states);
}
fs.writeFileSync(process.argv[4], JSON.stringify(out));
