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

  init = async (gdy) => {
    try {
      this.module = await new Module(this.moduleOverrides);
      this.jiddly = new this.module.Jiddly();
      this.gdy = this.jiddly.loadString(gdy);

      this.game = this.gdy.createGame("Vector");

      this.game.registerPlayer("player", "Vector");

      this.game.init();
      this.game.reset();

      return this.game.getState();
    } catch (error) {
      console.error(error);
    }
  };

  getPlayerObservations = () => {};

  getState = async () => {
    try {
      return this.game.getState();
    } catch (error) {
      console.error(error);
    }
  };

  envStep = async (action) => {
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

  envReset = async () => {
    try {
      return this.game.reset();
    } catch (error) {
      console.error(error);
    }
  };
}

export default JiddlyCore;
