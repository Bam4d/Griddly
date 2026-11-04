import Module from "./wasm/griddlyjs.js";

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
    if (this.game) {
      this.game.release();
      this.game.delete();
      delete this.game;
    }
    if(this.gdy) {
      this.gdy.delete();
      delete this.gdy;
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

  getExceptionMessage = (messagePtr) => {
    return this.griddlyjs.getExceptionMessage(messagePtr);
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

  loadGame = (gamePath, observerName = "Sprite2D", levelId = 0) => {
    console.log("loadGame:", gamePath, observerName, levelId);
    this.unloadGDY();
    const gdyString = this.readFile(gamePath);
    console.log("gdy read ok, len", gdyString.length);
    this.gdy = this.griddlyjs.loadString(gdyString);
    this.game = this.gdy.createGame(observerName);
    this.playerCount = this.gdy.getPlayerCount();
    this.players = [];
    for (let p = 0; p < this.playerCount; p++) {
      this.players.push(this.game.registerPlayer("Player " + p, observerName));
    }
    this.game.init();
    this.game.loadLevel(levelId);
    this.game.reset();
    this.observe();
    console.log("loadGame done");
  };

  observe = () => {
    return this.game.observe();
  };

  // Zero-copy RGBA pixels from the SDL render buffer, plus {width, height}
  observeImage = async () => {
    const desc = this.game.getGlobalObservationDescription();
    const shape = Array.from(desc.Shape); // [3, width, height]
    const pixels: Uint8Array = await Promise.resolve(this.game.observeImage()); // Uint8Array (w*h*4)
    return { pixels, width: shape[1], height: shape[2] };
  };

  getLevelCount = () => {
    return this.gdy.getLevelCount();
  };
}

export default GriddlyJSCore;
