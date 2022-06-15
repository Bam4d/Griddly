import Module from "./wasm/jiddly.js";

/**
 * Contains all the code for loading and interfacing with Jiddly
 */
class JiddlyCore {
  constructor() {
    this.moduleOverrides = {
      locateFile: (file, prefix) => {
        if (file === "jiddly.wasm") {
          const newPath = prefix + "../../js/" + file;
          console.log("new path:", newPath);
          return newPath;
        }
      },
    };
  }

  init = async () => {
    this.module = await new Module(this.moduleOverrides);
    this.jiddly = new this.module.Jiddly();
  };

  loadGDY = (gdyString) => {
    this.gdy = this.jiddly.loadString(gdyString);
    this.game = this.gdy.createGame("Vector");

    this.playerCount = this.gdy.getPlayerCount();

    this.players = [];

    for (let p = 0; p < this.playerCount; p++) {
      const player = this.game.registerPlayer("Player " + p, "Vector");
      this.players.push(player);
    }

    this.game.init();
    this.game.reset();
  };

  unloadGDY = () => {
    if (this.game) {
      this.game.release();
      this.game.delete();
    }
    if(this.gdy) {
      this.gdy.delete();
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
    if (levelStringOrId) {
      if (isNaN(levelStringOrId)) {
        this.game.loadLevelString(levelStringOrId);
      } else {
        this.game.loadLevel(levelStringOrId);
      }
    }
    return this.game.reset();
  };

  seed = (seed) => {
    this.game.seedRandomGenerator(seed);
  }
}

export default JiddlyCore;
