// node studio/griddly-demo/tools/remaster/memekit_check.mjs [gdy=/tmp/memekit_anim_test.yaml] [--png out.png]
// Loads a GDY using memekit sprites (memes/*.png are written into the WASM FS first, since the
// prebuilt griddlyjs.data doesn't have them), steps it and prints each animated object's
// renderTileId per tick, asserting logo/fire/rack cycle as configured; renders a Sprite2D frame.
import fs from "fs";
import path from "path";
import zlib from "zlib";
import { fileURLToPath } from "url";
const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));
const args = process.argv.slice(2);
const gdyPath = args.find((a) => a.endsWith(".yaml")) ?? "/tmp/memekit_anim_test.yaml";
const pngOut = args.includes("--png") ? args[args.indexOf("--png") + 1] : null;
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
const FS = g.module.FS;
for (const sub of ["memes", "remastered"]) {
  const dir = path.join(root, "resources/images", sub);
  if (!fs.existsSync(dir)) continue;
  if (!FS.analyzePath(`/resources/images/${sub}`).exists) FS.mkdir(`/resources/images/${sub}`);
  for (const f of fs.readdirSync(dir)) if (f.endsWith(".png")) FS.writeFile(`/resources/images/${sub}/${f}`, fs.readFileSync(path.join(dir, f)));
}
g.loadGDYString(fs.readFileSync(gdyPath, "utf8"));
g.createGame("Sprite2D");
g.loadLevel(0);
g.seed(1);
g.reset();
const tiles = (n) => g.getState().objects.filter((o) => o.name === n).map((o) => o.renderTileId)[0];
const seq = { logo: [], fire: [], rack: [], bufo: [], bufoX: [] };
const moves = [3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0];  // walk onto the fire floor (Z0) and back
for (let t = 0; t < 12; t++) {
  for (const k of ["logo", "fire", "rack", "bufo"]) seq[k].push(tiles(k));
  seq.bufoX.push(g.getState().objects.find((o) => o.name === "bufo").location[0]);
  g.step(moves[t]);
}
for (const [k, v] of Object.entries(seq)) console.log(`${k.padEnd(5)} ${v.join(" ")}`);
const cyc = (v, set) => new Set(v).size === set.length && set.every((x) => v.includes(x));
let ok = cyc(seq.logo, [0, 1, 2, 3]) && cyc(seq.fire, [0, 1, 2]) && cyc(seq.rack, [1, 2]);
// fire keeps cycling while the bufo stands on it (ticks 1..4)
ok &&= new Set(seq.fire.slice(2, 5)).size === 3;
const im = await g.observeImage();
let lit = 0;
for (let i = 0; i < im.pixels.length; i += 4) if (im.pixels[i] + im.pixels[i + 1] + im.pixels[i + 2] > 30) lit++;
console.log(`Sprite2D ${im.width}x${im.height} lit ${(100 * lit / (im.width * im.height)).toFixed(0)}%`);
if (pngOut) {
  const { pixels, width, height } = im;
  const raw = Buffer.alloc((width * 4 + 1) * height);
  for (let y = 0; y < height; y++) Buffer.from(pixels.buffer, pixels.byteOffset + y * width * 4, width * 4).copy(raw, y * (width * 4 + 1) + 1);
  const T = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
  const crc = (b) => { let c = 0xffffffff; for (const x of b) c = T[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
  const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
  const ih = Buffer.alloc(13); ih.writeUInt32BE(width, 0); ih.writeUInt32BE(height, 4); ih[8] = 8; ih[9] = 6;
  fs.writeFileSync(pngOut, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ih), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]));
}
console.log(ok ? "ANIM OK" : "ANIM FAIL");
process.exit(ok ? 0 : 1);
