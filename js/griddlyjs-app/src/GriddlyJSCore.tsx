import Module from "./wasm/griddlyjs.js";
import yaml from "js-yaml";

/**
 * Contains all the code for loading and interfacing with GriddlyJS
 */
class GriddlyJSCore {
  constructor() {
    this.moduleOverrides = {
      locateFile: (file, prefix) => {
        // wasm + preloaded resources data live in public/js (served at /js/)
        if (file === "griddlyjs.wasm" || file === "griddlyjs.data") {
          const newPath = "/js/" + file;
          console.log("locateFile:", file, "->", newPath);
          return newPath;
        }
        return (prefix || "/") + file;
      },
    };
  }

  init = () => {
    // emscripten module factories are not reentrant (StrictMode double-invokes
    // componentDidMount in dev), so share a single init promise
    if (!this.initPromise) {
      this.initPromise = (async () => {
        this.module = await Module(this.moduleOverrides);
        this.griddlyjs = new this.module.GriddlyJS();
      })();
    }
    return this.initPromise;
  };

  loadGDY = (gdyString) => {
    try {
      this.gdy = this.griddlyjs.loadString(gdyString);
      this.game = this.gdy.createGame("Vector");

      this.playerCount = this.gdy.getPlayerCount();

      this.players = [];

      for (let p = 0; p < this.playerCount; p++) {
        const player = this.game.registerPlayer("Player " + p, "Vector");
        this.players.push(player);
      }

      this.game.init();
      this.game.reset();
    } catch(e) {
      if(!isNaN(e)) {
        throw Error(this.griddlyjs.getExceptionMessage(e));
      }
      throw e;
    }
  };

  unloadGDY = () => {
    try {
      if (this.game) {
        this.game.release();
        this.game.delete();
        delete this.game;
      }
      if (this.gdy) {
        this.gdy.delete();
        delete this.gdy;
      }
    } catch (e) {
      console.log("unloadGDY error:", String(e));
    }
  };

  getPlayerObservations = (playerId) => {
    return this.players[playerId].observe();
  };

  getActionInputMappings = () => {
    return this.gdy.getActionInputMappings();
  };

  getActionNames = () => {
    return this.gdy.getExternalActionNames();
  };

  getGlobalVariables = () => {
    const globalVariableNames = this.game.getGlobalVariableNames();
    return this.game.getGlobalVariables(globalVariableNames);
  };

  getState = () => {
    return this.game.getState();
  };

  getHeight = () => {
    return this.game.getHeight();
  };

  getWidth = () => {
    return this.game.getWidth();
  };

  step = (action) => {
    try {
      const playerActions = [];
      if (!Array.isArray(action)) {
        playerActions.push([action]);
      } else if (!Array.isArray(action[0])) {
        playerActions.push(action);
      }

      const actionLength = playerActions[0].length;

      for (let p = 1; p < this.playerCount; p++) {
        playerActions.push(new Array(actionLength).fill(0));
      }

      return this.game.stepParallel(playerActions);
    } catch(e){
      if(!isNaN(e)) {
        throw Error(this.griddlyjs.getExceptionMessage(e));
      }
      throw e;
    }
  };

  reset = (levelStringOrId) => {
    if(!this.game) {
      return;
    }
    if (levelStringOrId) {
      this.levelStringOrId = levelStringOrId;
      if (isNaN(levelStringOrId)) {
        this.game.loadLevelString(levelStringOrId);
      } else {
        this.game.loadLevel(levelStringOrId);
      }
    }
    return this.game.reset();
  };

  getLevelStringOrId = () => {
    return this.levelStringOrId;
  }

  seed = (seed) => {
    this.game.seedRandomGenerator(seed);
  }

  getExceptionMessage = (e) => {
    // embind throws C++ exceptions as { excPtr } (or a raw pointer); anything else is a JS error
    const ptr = typeof e === "number" ? e : e && typeof e.excPtr === "number" ? e.excPtr : null;
    if (ptr !== null) {
      try {
        return this.griddlyjs.getExceptionMessage(ptr);
      } catch {
        return "C++ exception (ptr " + ptr + ")";
      }
    }
    if (e && e.message) return String(e.message);
    return String(e);
  }

  // Render straight into the page's <canvas id="canvas"> via SDL (WebGL/WebGPU)
  enableCanvasRendering = () => {
    this.module.setSDLWindowVisible(true);
  };

  readFile = (path) => {
    return this.module.FS.readFile(path, { encoding: "utf8" });
  };

  listYamlFiles = (dir, out = []) => {
    for (const entry of this.module.FS.readdir(dir)) {
      if (entry === "." || entry === "..") continue;
      const full = dir + "/" + entry;
      const stat = this.module.FS.stat(full);
      if (this.module.FS.isDir(stat.mode)) {
        this.listYamlFiles(full, out);
      } else if (entry.endsWith(".yaml")) {
        out.push(full);
      }
    }
    return out;
  };

  // Render-capable observers this game actually defines (Sprite2D/Block2D/Isometric)
  getAvailableRenderObservers = () => {
    const OT = this.module.ObserverType;
    const renderTypes = new Set([OT.SPRITE_2D, OT.BLOCK_2D, OT.ISOMETRIC]);
    const names = Array.from(this.gdy.getDefinedObserverNames());
    return names.filter((n) => renderTypes.has(this.gdy.getObserverType(n)));
  };

  // Parse the GDY file only (lets the UI query supported observers before creating a game)
  loadGDYFile = (gamePath) => {
    this.unloadGDY();
    const gdyString = this.readFile(gamePath);
    this.gdy = this.griddlyjs.loadString(gdyString);
    // level count straight from the YAML (embind getLevelCount is unreliable here)
    try {
      const doc: any = yaml.load(gdyString);
      const levels = doc?.Environment?.Levels;
      this.levelCount = Array.isArray(levels) ? levels.length : 1;
    } catch {
      this.levelCount = 1;
    }
  };

  // Create + start the game with a given observer name
  startGame = (observerName = "Sprite2D", levelId = 0) => {
    this.game = this.gdy.createGame(observerName);
    this.playerCount = this.gdy.getPlayerCount();
    this.players = [];
    for (let p = 0; p < this.playerCount; p++) {
      this.players.push(this.game.registerPlayer("Player " + p, observerName));
    }
    this.game.init();
    this.game.loadLevel(levelId);
    this.game.reset(); // always reset after loading a level
    this.observe();
  };

  // Parse a GDY just to read its player count (without creating a game)
  peekPlayerCount = (gamePath) => {
    const gdy = this.griddlyjs.loadString(this.readFile(gamePath));
    const n = gdy.getPlayerCount();
    gdy.delete();
    return n;
  };

  loadGame = (gamePath, observerName = "Sprite2D", levelId = 0) => {
    this.loadGDYFile(gamePath);
    this.startGame(observerName, levelId);
  };

  observe = () => {
    return this.game.observe();
  };

  // Zero-copy RGBA pixels from the SDL render buffer, plus {width, height}
  observeImage = async () => {
    const desc = this.game.getGlobalObservationDescription();
    const pixels: Uint8Array = await Promise.resolve(this.game.observeImage()); // Uint8Array (w*h*4)
    // Shape is unset until the observer has rendered once; fall back to square
    if (desc.Shape) {
      const shape = Array.from(desc.Shape); // [3, width, height]
      return { pixels, width: shape[1], height: shape[2] };
    }
    const side = Math.sqrt(pixels.length / 4) | 0;
    return { pixels, width: side, height: side };
  };

  getLevelCount = () => {
    return this.gdy.getLevelCount();
  };
}

export default GriddlyJSCore;
