// Render a level of an office meme game to PNG (memes/ + remastered/ sprites are written into the WASM
// FS first, since the prebuilt griddlyjs.data doesn't have them yet):
//   node office_render.mjs <game_id> <level> <out.png> [observer=Sprite2D] [actions.json: [[name, id], ...]]
import fs from "fs";
import path from "path";
import zlib from "zlib";
import { fileURLToPath } from "url";
const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../../..");
const { createGriddly } = await import(path.join(root, "studio/griddly-demo/packages/ts/griddly-wasm/dist/index.js"));
const [id, level, out, obs = "Sprite2D", actsFile] = process.argv.slice(2);
const g = await createGriddly({ moduleOverrides: { print() {}, printErr() {} } });
const FS = g.module.FS;
for (const sub of ["memes", "remastered"]) {
  const dir = path.join(root, "resources/images", sub);
  if (!fs.existsSync(dir)) continue;
  if (!FS.analyzePath(`/resources/images/${sub}`).exists) FS.mkdir(`/resources/images/${sub}`);
  for (const f of fs.readdirSync(dir)) if (f.endsWith(".png")) FS.writeFile(`/resources/images/${sub}/${f}`, fs.readFileSync(path.join(dir, f)));
}
g.loadGDYString(fs.readFileSync(path.join(root, "resources/games/Single-Player/Remastered", `${id}.yaml`), "utf8"));
g.createGame(obs);
g.loadLevel(Number(level));
g.seed(1);
g.reset();
const names = g.getActionNames();
if (actsFile) for (const [a, i] of JSON.parse(fs.readFileSync(actsFile, "utf8"))) g.step(names.length > 1 ? [names.indexOf(a), i] : [i]);
const { pixels, width, height } = await g.observeImage();
let lit = 0;
for (let i = 0; i < pixels.length; i += 4) if (pixels[i] + pixels[i + 1] + pixels[i + 2] > 30) lit++;
console.log(`${id} L${level} ${obs} ${width}x${height} lit ${(100 * lit / (width * height)).toFixed(0)}%`);
const raw = Buffer.alloc((width * 4 + 1) * height);
for (let y = 0; y < height; y++) Buffer.from(pixels.buffer, pixels.byteOffset + y * width * 4, width * 4).copy(raw, y * (width * 4 + 1) + 1);
const T = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
const crc = (b) => { let c = 0xffffffff; for (const x of b) c = T[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(width, 0); ihdr.writeUInt32BE(height, 4); ihdr[8] = 8; ihdr[9] = 6;
fs.writeFileSync(out, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ihdr), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]));
