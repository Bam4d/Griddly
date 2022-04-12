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

      this.game = await this.gdy.createGame("Vector");

      await this.game.registerPlayer("player", "Vector");

      await this.game.init();
      await this.game.reset();

      return await this.game.getState();
    } catch (error) {
      console.error(error);
    }
  };

  getPlayerObservations = () => {};

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
