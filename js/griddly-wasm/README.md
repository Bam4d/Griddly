# @griddly/wasm

The [Griddly](https://github.com/bam4d/Griddly) grid-world game engine compiled to
WebAssembly (emscripten + embind), packaged as an npm-style module with a typed
TypeScript API. Runs in the browser (Vite, webpack, ...) and in Node.js.

## Install

From GitHub (branch + subdirectory):

```bash
npm install "github:bam4d/Griddly#vibe_main:js/griddly-wasm"
# or a tagged release / specific commit:
npm install "github:bam4d/Griddly#<ref>:js/griddly-wasm"
```

## Usage

```ts
import { createGriddly } from "@griddly/wasm";

const griddly = await createGriddly();

// Load a game definition (GDY YAML). The engine ships its built-in games
// preloaded in the WASM virtual FS under /resources:
griddly.loadGDYFile("/resources/games/Single-Player/Mini-Grid/minigrid-doggo.yaml");
// or from a YAML string:
// griddly.loadGDYString(gdyYaml);

const game = griddly.createGame("Vector"); // headless observer
griddly.loadLevel(0);
griddly.seed(42);
griddly.reset();

const obs = griddly.observe();
griddly.step([2]); // action input for player 1
```

Rendered (Sprite2D/Block2D/Isometric) observers draw into an SDL canvas:

```ts
const griddly = await createGriddly();
griddly.loadGDYFile("/resources/games/Single-Player/GVGAI/zelda.yaml");
griddly.createGame("Sprite2D");
griddly.loadLevel(0);
griddly.reset();
griddly.enableCanvasRendering();
```

## Assets

The package ships three files:

| File             | Purpose                                             |
| ---------------- | --------------------------------------------------- |
| `griddlyjs.js`   | emscripten ES6 module factory (default export)      |
| `griddlyjs.wasm` | compiled engine                                     |
| `griddlyjs.data` | preloaded game resources (`/resources/...`)        |

`createGriddly()` resolves them automatically relative to the package
(`import.meta.url`). In bundlers, make sure `.wasm`/`.data` are treated as
assets (Vite does this by default for imported URLs). To override resolution,
pass `locateFile`:

```ts
await createGriddly({ locateFile: (f) => `/assets/${f}` });
```

## Rebuilding the artifacts

From the repo root with the [emscripten SDK](https://emscripten.org) active
(`EMSDK=/path/to/emsdk`):

```bash
js/build.sh Release
cp build_wasm/bin/griddlyjs.{js,wasm,data} js/griddly-wasm/
```
