import Module from "./wasm/griddlyjs.js";

/**
 * Contains all the code for loading and interfacing with GriddlyJS
 */
class GriddlyJSCore {
  constructor() {
    this.moduleOverrides = {
      locateFile: (file, prefix) => {
        if (file === "griddlyjs.wasm") {
          const newPath = prefix + "../../js/" + file;
          console.log("new path:", newPath);
          return newPath;
        }
      },
    };
  }

  init = async () => {
    this.module = await new Module(this.moduleOverrides);
    this.griddlyjs = new this.module.GriddlyJS();
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
}

export default GriddlyJSCore;
