// Engine smoke test for a remastered game, run with node from the repo:
//   node studio/griddly-demo/tools/remaster/engine_check.mjs <game_id> [episodes] [maxSteps]
//
// Loads resources/games/Single-Player/Remastered/<game_id>.yaml in the WASM engine (needs
// studio/griddly-demo/packages/ts/griddly-wasm, i.e. `bun run sync-wasm` once), then for every level:
//   * loads/resets it and reports size + object count
//   * checks the avatar starts facing down (render tile 2) and that moving changes its facing tile
//     (games whose `move` action is Relative - egocentric avatars like torchlight / nightgarden - are
//     detected automatically and checked instead for: a set initial orientation (A[U] etc.), 1/3
//     turning left/right, 2 stepping forward along the facing)
//   * runs `episodes` episodes of a random-ish agent (biased to keep moving) for up to maxSteps,
//     reporting wins / losses / timeouts using the GDY's own termination (step() result)
// Exits non-zero if a level fails to load, the avatar is missing, or facing doesn't work.
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));

const id = process.argv[2];
const episodes = Number(process.argv[3] ?? 10);
const maxSteps = Number(process.argv[4] ?? 400);
const gdyPath = path.join(root, "resources/games/Single-Player/Remastered", `${id}.yaml`);
const gdy = fs.readFileSync(gdyPath, "utf8");
const avatarName = gdy.match(/AvatarObject:\s*(\S+)/)[1];

const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
g.loadGDYString(gdy);
g.createGame("Vector");
const n = g.getLevelCount();
const actions = g.getActionNames();
console.log(`${id}: ${n} levels, actions=${actions.join(",")}, avatar=${avatarName}`);

const avatar = () => g.getState().objects.find((o) => o.name === avatarName);
const moveIdx = actions.indexOf("move");
// step() with a 2-element action [actionTypeIdx, actionId] when there is more than one action
const act = (name, id) => (actions.length > 1 ? [actions.indexOf(name), id] : [id]);

const relative = !!g.getActionInputMappings()?.move?.relative;
if (relative) console.log("  (relative move action: checking orientation instead of facing tiles)");
const DIRV = { UP: [0, -1], RIGHT: [1, 0], DOWN: [0, 1], LEFT: [-1, 0] };
const ORDER = ["UP", "RIGHT", "DOWN", "LEFT"];

let failed = false;
for (let l = 0; l < n; l++) {
  g.loadLevel(l);
  g.seed(1);
  g.reset();
  const a0 = avatar();
  if (!a0) {
    console.log(`  L${l}: NO AVATAR`);
    failed = true;
    continue;
  }
  let startTile = a0.renderTileId;
  let facingOk = false;
  if (relative) {
    // initial orientation set; 1 turns left, 3 turns right; 2 moves forward (when not blocked)
    const o0 = a0.orientation;
    startTile = o0;
    const turned = (id) => { g.reset(); g.step(act("move", id)); return avatar()?.orientation; };
    const i0 = ORDER.indexOf(o0);
    const leftOk = i0 >= 0 && turned(1) === ORDER[(i0 + 3) % 4];
    const rightOk = i0 >= 0 && turned(3) === ORDER[(i0 + 1) % 4];
    let fwdOk = false;
    for (const pre of [[], [3], [3, 3], [1]]) {
      g.reset();
      for (const p of pre) g.step(act("move", p));
      const b = avatar();
      g.step(act("move", 2));
      const c = avatar();
      const v = DIRV[b.orientation];
      if (c && c.location[0] === b.location[0] + v[0] && c.location[1] === b.location[1] + v[1]) { fwdOk = true; break; }
    }
    facingOk = leftOk && rightOk && fwdOk;
    if (!facingOk) console.log(`  L${l}: relative controls broken: left=${leftOk} right=${rightOk} forward=${fwdOk}`);
    if (i0 < 0) failed = true;
  }
  else {
  // try each direction until one changes the facing tile
  for (const dir of [1, 2, 3, 4]) {
    g.reset();
    g.step(act("move", dir));
    const a = avatar();
    if (a && a.renderTileId === { 1: 3, 2: 0, 3: 1, 4: 2 }[dir]) facingOk = true;
  }
  if (startTile !== 2) failed = true;
  }
  if (!facingOk) failed = true;

  let wins = 0, losses = 0, timeouts = 0, totalSteps = 0;
  for (let ep = 0; ep < episodes; ep++) {
    g.seed(ep + 1);
    g.reset();
    let dir = 1 + (ep % 4);
    let done = false;
    for (let t = 0; t < maxSteps && !done; t++) {
      if (Math.random() < 0.25) dir = 1 + Math.floor(Math.random() * 4);
      const extra = actions.filter((x) => x !== "move" && !x.startsWith("_"));
      const useExtra = extra.length && Math.random() < 0.15;
      const r = g.step(useExtra ? act(extra[0], dir) : act("move", dir));
      if (r && r.terminated) {
        done = true;
        totalSteps += t + 1;
        const res = r.info?.playerResults?.["1"];
        if (res === "Win") wins++;
        else losses++;
      }
    }
    if (!done) timeouts++;
  }
  console.log(
    `  L${l}: ${g.width}x${g.height} objects=${g.getState().objects.length} ${relative ? "orientation" : "startTile"}=${startTile} facing=${facingOk ? "ok" : "BROKEN"} | random agent: win ${wins} lose ${losses} timeout ${timeouts}`
  );
}
process.exit(failed ? 1 : 0);
