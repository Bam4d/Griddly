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
    try {
      this.module = await new Module(this.moduleOverrides);
      this.jiddly = new this.module.Jiddly();
    } catch (error) {
      console.error(error);
    }
  };

  loadGDY = (gdyString) => {
    try {
      this.gdy = this.jiddly.loadString(gdyString);
      this.game = this.gdy.createGame("Vector");

      this.playerCount = this.gdy.getPlayerCount();

      for (let p = 0; p < this.playerCount; p++) {
        this.game.registerPlayer("Player " + p, "Vector");
      }

      this.game.init();
      this.game.reset();
    } catch (e) {
      console.log("Error loading GDY", e);
    }
  };

  unloadGDY = () => {
    this.game.release();
    this.gdy.delete();
    this.game.delete();
  };

  getPlayerObservations = () => {};

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
    try {
      return this.game.getState();
    } catch (error) {
      console.error(error);
    }
  };

  getHeight = () => {
    return this.game.getHeight();
  };

  getWidth = () => {
    return this.game.getWidth();
  };

  step = (action) => {
    try {
      if (!Array.isArray(action)) {
        action = [[action]];
      } else if (!Array.isArray(action[0])) {
        action = [action];
      }

      return this.game.stepParallel(action);
    } catch (error) {
      console.error(error);
    }
  };

  reset = () => {
    try {
      return this.game.reset();
    } catch (error) {
      console.error(error);
    }
  };
}

export default JiddlyCore;
