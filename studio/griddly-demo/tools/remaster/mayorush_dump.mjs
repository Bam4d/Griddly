// Helper for mayorush.py --diff: replays move lists in the WASM engine and writes the state after every
// move (bufo cell + tier, rack cells, win) as JSON, so the Python simulator can be checked against the GDY.
//   node mayorush_dump.mjs <gdy.yaml> <jobs.json> <out.json>   jobs = [{level, moves: [1..4, ...]}, ...]
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
const snap = (r) => {
  const s = g.getState();
  const b = s.objects.find((o) => o.name === "bufo");
  const racks = s.objects.filter((o) => o.name === "rack").map((o) => o.location).sort();
  return { pos: b.location, tier: b.variables.tier, tile: b.renderTileId, racks, won: !!(r?.terminated && r.info?.playerResults?.["1"] === "Win") };
};
const out = [];
for (const job of jobs) {
  g.loadLevel(job.level);
  g.seed(1);
  g.reset();
  const states = [snap(null)];
  for (const m of job.moves) {
    const r = g.step([m]);
    states.push(snap(r));
    if (r?.terminated) break;
  }
  out.push(states);
}
fs.writeFileSync(process.argv[4], JSON.stringify(out));
